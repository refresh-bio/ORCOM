#include "Globals.h"

#include <cstring>
#if (DEV_PRINT_STATS)
#	include <iostream>	// todo: move printing outside
#endif

#include "BinFile.h"
#include "BitMemory.h"
#include "BinBlockData.h"
#include "Exception.h"
#include "BitMemory.h"


BinFileWriter::BinFileWriter()
	:	metaStream(NULL)
	,	dnaStream(NULL)
	,	currentBlockId(0)
	,	minimizersCount(0)
{		
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(BinFileHeader), 0);
}

BinFileWriter::~BinFileWriter()
{
	if (metaStream != NULL)
		delete metaStream;

	if (dnaStream != NULL)
		delete dnaStream;
}

void BinFileWriter::StartCompress(const std::string& fileName_, const BinModuleConfig& params_)
{
	ASSERT(metaStream == NULL);
	ASSERT(dnaStream == NULL);

	metaStream = new FileStreamWriter(fileName_ + ".bmeta");
	((FileStreamWriter*)metaStream)->SetBuffering(true);

	dnaStream = new FileStreamWriter(fileName_ + ".bdna");
	((FileStreamWriter*)dnaStream)->SetBuffering(true);


	// clear header and footer
	//
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(BinFileHeader), 0);

	fileFooter.Clear();
	fileFooter.params = params_;
	minimizersCount = params_.minimizer.TotalMinimizersCount();


	// skip header pos
	//
	metaStream->SetPosition(BinFileHeader::HeaderSize);

	currentBlockId = 0;
}

void BinFileWriter::WriteNextBlock(const BinaryBinBlock* block_)
{
	ASSERT(block_ != NULL);
	ASSERT(block_->descriptors.size() == minimizersCount + 1);

	for (uint32 i = 0; i < block_->descriptors.size(); ++i)
	{
		const BinaryBinDescriptor& desc = block_->descriptors[i];

		fileFooter.metaBinSizes.push_back(desc.metaSize);			// we can optimize this further -- to use bit map
		if (desc.metaSize > 0)
		{
			ASSERT(desc.recordsCount > 0);

			fileHeader.recordsCount += desc.recordsCount;

			fileFooter.dnaBinSizes.push_back(desc.dnaSize);
			fileFooter.recordsCounts.push_back(desc.recordsCount);
			fileFooter.rawDnaSizes.push_back(desc.rawDnaSize);
		}
	}

	metaStream->Write(block_->metaData.Pointer(), block_->metaSize);
	dnaStream->Write(block_->dnaData.Pointer(), block_->dnaSize);

	currentBlockId++;
}

void BinFileWriter::FinishCompress()
{
	ASSERT(metaStream != NULL);
	ASSERT(dnaStream != NULL);
	ASSERT(fileFooter.metaBinSizes.size() > 0);
	ASSERT(fileFooter.metaBinSizes.size() == currentBlockId * (minimizersCount + 1));

	// prepare header and footer
	//
	std::fill(fileHeader.reserved, fileHeader.reserved + BinFileHeader::ReservedBytes, 0);
	fileHeader.blockCount = currentBlockId;
	fileHeader.footerOffset = metaStream->Position();

	WriteFileFooter();
	fileHeader.footerSize = metaStream->Position() - fileHeader.footerOffset;

	metaStream->SetPosition(0);
	WriteFileHeader();

#if (DEV_PRINT_STATS)
	// display bins ------------- TODO move outside
	//
	{
		std::cerr << "Packed bin sizes:\n[sig_id] : rec_count ( meta_size , dna_size )\n";

		const uint32 minCount = fileFooter.params.minimizer.TotalMinimizersCount() + 1;
		std::vector<uint64> recCount, metaSize, dnaSize;
		recCount.resize(minCount);
		metaSize.resize(minCount);
		dnaSize.resize(minCount);

		uint64 rcIter = 0;
		for (uint64 i = 0; i < fileFooter.metaBinSizes.size(); ++i)
		{
			uint32 ms = fileFooter.metaBinSizes[i];
			if (ms > 0)
			{
				uint32 minId = i % minCount;
				recCount[minId] += fileFooter.recordsCounts[rcIter];
				metaSize[minId] += fileFooter.metaBinSizes[rcIter];
				dnaSize[minId] += fileFooter.dnaBinSizes[rcIter];
				rcIter++;
			}
		}
		for (uint64 i = 0; i < recCount.size(); ++i)
		{
			if (recCount[i] > 0)
			{
				std::cerr << '[' << i << "] : " << recCount[i]
							 << " ( " << metaSize[i]
							 << " , " << dnaSize[i] << " )\n";
			}
		}
		std::cerr << std::endl;
	}
#endif


	// cleanup
	//
	metaStream->Close();
	dnaStream->Close();

	delete metaStream;
	delete dnaStream;

	metaStream = NULL;
	dnaStream = NULL;
}

void BinFileWriter::WriteFileHeader()
{
	metaStream->Write((byte*)&fileHeader, BinFileHeader::HeaderSize);
}

void BinFileWriter::WriteFileFooter()
{
	const uint64 osize = BinFileFooter::ParametersSize + 8 +
			(fileFooter.metaBinSizes.size() + fileFooter.dnaBinSizes.size() +
			 fileFooter.recordsCounts.size() + fileFooter.rawDnaSizes.size()) * sizeof(uint32);

	Buffer obuf(osize);
	BitMemoryWriter writer(obuf);
	writer.PutBytes((byte*)&fileFooter.params, BinFileFooter::ParametersSize);
	writer.Put8Bytes(fileFooter.dnaBinSizes.size());

	// store blocks
	//
	writer.PutBytes((byte*)fileFooter.metaBinSizes.data(), fileFooter.metaBinSizes.size() * sizeof(uint32));
	writer.PutBytes((byte*)fileFooter.dnaBinSizes.data(), fileFooter.dnaBinSizes.size() * sizeof(uint32));
	writer.PutBytes((byte*)fileFooter.recordsCounts.data(), fileFooter.recordsCounts.size() * sizeof(uint32));
	writer.PutBytes((byte*)fileFooter.rawDnaSizes.data(), fileFooter.rawDnaSizes.size() * sizeof(uint32));

	metaStream->Write(writer.Pointer(), writer.Position());
}

BinFileReader::BinFileReader()
	:	metaStream(NULL)
	,	dnaStream(NULL)
	,	currentBlockId(0)
	,	minimizersCount(0)
	,	currentNonEmptyBlockId(0)
{
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(BinFileHeader), 0);
}

BinFileReader::~BinFileReader()
{
	if (metaStream)
		delete metaStream;

	if (dnaStream)
		delete dnaStream;
}

void BinFileReader::StartDecompress(const std::string& fileName_, BinModuleConfig& params_)
{
	ASSERT(metaStream == NULL);
	ASSERT(dnaStream == NULL);

	metaStream = new FileStreamReader(fileName_ + ".bmeta");
	((FileStreamReader*)metaStream)->SetBuffering(true);
	dnaStream = new FileStreamReader(fileName_ + ".bdna");
	((FileStreamReader*)dnaStream)->SetBuffering(true);

	if (metaStream->Size() == 0)
		throw Exception("Empty file.");

	// read header
	//
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(BinFileHeader), 0);
	ReadFileHeader();

	if ((fileHeader.blockCount == 0ULL) || fileHeader.footerOffset + (uint64)fileHeader.footerSize > metaStream->Size())
	{
		delete metaStream;
		delete dnaStream;
		metaStream = NULL;
		dnaStream = NULL;
		throw Exception("Corrupted archive header");
	}


	// read footer
	//
	fileFooter.Clear();

	metaStream->SetPosition(fileHeader.footerOffset);
	ReadFileFooter();
	params_ = fileFooter.params;

	metaStream->SetPosition(BinFileHeader::HeaderSize);

	currentBlockId = 0;
	currentNonEmptyBlockId = 0;
}

bool BinFileReader::ReadNextBlock(BinaryBinBlock* block_)
{
	ASSERT(block_ != NULL);

	if (currentBlockId == fileHeader.blockCount)
		return false;

	if (block_->descriptors.size() < minimizersCount + 1)
		block_->descriptors.resize(minimizersCount + 1);

	uint64 blockIdx = currentBlockId * (minimizersCount + 1);
	uint64 totalMetaSize = 0;
	uint64 totalDnaSize = 0;
	uint64 totalRawDnaSize = 0;

	for (uint32 i = 0; i < minimizersCount + 1; ++i)
	{
		uint64 binIdx = blockIdx + i;

		BinaryBinDescriptor& desc = block_->descriptors[i];
		desc.metaSize = fileFooter.metaBinSizes[binIdx];

		if (desc.metaSize == 0)
		{
			desc.dnaSize = 0;
			desc.recordsCount = 0;
			desc.rawDnaSize = 0;
			continue;
		}

		ASSERT(currentNonEmptyBlockId < fileFooter.dnaBinSizes.size());
		desc.dnaSize = fileFooter.dnaBinSizes[currentNonEmptyBlockId];
		desc.recordsCount = fileFooter.recordsCounts[currentNonEmptyBlockId];
		desc.rawDnaSize = fileFooter.rawDnaSizes[currentNonEmptyBlockId];
		currentNonEmptyBlockId++;

		totalMetaSize += desc.metaSize;
		totalDnaSize += desc.dnaSize;
		totalRawDnaSize += desc.rawDnaSize;
	}

	block_->metaSize = totalMetaSize;
	block_->dnaSize = totalDnaSize;
	block_->rawDnaSize = totalRawDnaSize;

	if (totalMetaSize > 0)
	{
		if (block_->metaData.Size() < totalMetaSize)
			block_->metaData.Extend(totalMetaSize);

		if (block_->dnaData.Size() < totalDnaSize)
			block_->dnaData.Extend(totalDnaSize);

		metaStream->Read(block_->metaData.Pointer(), totalMetaSize);
		dnaStream->Read(block_->dnaData.Pointer(), totalDnaSize);
	}

	currentBlockId++;

	return true;
}

void BinFileReader::FinishDecompress()
{
	if (metaStream)
	{
		metaStream->Close();
		delete metaStream;
		metaStream = NULL;
	}

	if (dnaStream)
	{
		dnaStream->Close();
		delete dnaStream;
		dnaStream = NULL;
	}
}

void BinFileReader::ReadFileHeader()
{
	metaStream->Read((byte*)&fileHeader, BinFileHeader::HeaderSize);
}

void BinFileReader::ReadFileFooter()
{
	Buffer buffer(fileHeader.footerSize);

	metaStream->Read(buffer.Pointer(), fileHeader.footerSize);

	BitMemoryReader reader(buffer, fileHeader.footerSize);

	reader.GetBytes((byte*)&fileFooter.params, BinFileFooter::ParametersSize);
	minimizersCount = fileFooter.params.minimizer.TotalMinimizersCount();

	// read blocks
	//
	fileFooter.metaBinSizes.resize(fileHeader.blockCount * (minimizersCount + 1), 0);

	const uint64 bsize = reader.Get8Bytes();
	fileFooter.dnaBinSizes.resize(bsize, 0);
	fileFooter.recordsCounts.resize(bsize, 0);
	fileFooter.rawDnaSizes.resize(bsize, 0);

	reader.GetBytes((byte*)fileFooter.metaBinSizes.data(), fileFooter.metaBinSizes.size() * sizeof(uint32));
	reader.GetBytes((byte*)fileFooter.dnaBinSizes.data(), fileFooter.dnaBinSizes.size() * sizeof(uint32));
	reader.GetBytes((byte*)fileFooter.recordsCounts.data(), fileFooter.recordsCounts.size() * sizeof(uint32));
	reader.GetBytes((byte*)fileFooter.rawDnaSizes.data(), fileFooter.rawDnaSizes.size() * sizeof(uint32));
}
