/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "../orcom_bin/Globals.h"

#include <string>

#include "DnarchFile.h"
#include "CompressedBlockData.h"
#include "../orcom_bin/Exception.h"

#if (DEV_DEBUG_PRINT)
#	include <iostream>
#endif

DnarchFileWriter::DnarchFileWriter()
	:	metaStream(NULL)
	,	dataStream(NULL)
{}


DnarchFileWriter::~DnarchFileWriter()
{
	if (metaStream != NULL)
		delete metaStream;

	if (dataStream != NULL)
		delete dataStream;
}


void DnarchFileWriter::StartCompress(const std::string &fileName_, const MinimizerParameters &minParams_, const CompressorParams& compParams_)
{
	ASSERT(metaStream == NULL);

	metaStream = new FileStreamWriter(fileName_ + ".cmeta");
	metaStream->SetBuffering(true);

	dataStream = new FileStreamWriter(fileName_ + ".cdna");

	streamSizes.resize(DnaCompressedBin::BuffersNum, 0);


	// clear header and footer
	//
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(DnarchFileHeader), 0);
	fileHeader.minParams = minParams_;
	compParams = compParams_;

	fileFooter.blockSizes.clear();


	// skip header pos
	//
	metaStream->SetPosition(DnarchFileHeader::HeaderSize);
}


void DnarchFileWriter::WriteNextBin(const CompressedDnaBlock *bin_)
{
	ASSERT(bin_->dataBuffer.size > 0);

	for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
	{
		streamSizes[i] += bin_->bufferSizes[i];
	}

	fileFooter.blockSizes.push_back(bin_->dataBuffer.size);

	dataStream->Write(bin_->dataBuffer.data.Pointer(), bin_->dataBuffer.size);
}


void DnarchFileWriter::FinishCompress()
{
	ASSERT(metaStream != NULL);
	ASSERT(dataStream != NULL);


#if (DEV_DEBUG_PRINT)
	std::cerr << "--------.--------*--------.--------\n";
	std::cerr << "Compressed stream sizes: \n";
	std::cerr << "Flag " << streamSizes[DnaCompressedBin::FlagBuffer] << '\n';
	std::cerr << "LetterX " << streamSizes[DnaCompressedBin::LetterXBuffer] << '\n';
	std::cerr << "Rev " << streamSizes[DnaCompressedBin::RevBuffer] << '\n';
	std::cerr << "HardReads " << streamSizes[DnaCompressedBin::HardReadsBuffer] << '\n';
	std::cerr << "LzId " << streamSizes[DnaCompressedBin::LzIdBuffer] << '\n';
	std::cerr << "Shift " << streamSizes[DnaCompressedBin::ShiftBuffer] << '\n';
	std::cerr << "Len " << streamSizes[DnaCompressedBin::LenBuffer] << '\n';
	std::cerr << "Match " << streamSizes[DnaCompressedBin::MatchBuffer] << std::endl;
#endif

	// prepare header and write footer
	//
	fileHeader.footerOffset = metaStream->Position();

	WriteFileFooter();


	// fill header and write
	//
	fileHeader.footerSize = metaStream->Position() - fileHeader.footerOffset;

	metaStream->SetPosition(0);
	WriteFileHeader();


	// cleanup exit
	//
	metaStream->Close();
	delete metaStream;
	metaStream = NULL;

	dataStream->Close();
	delete dataStream;
	dataStream = NULL;
}


void DnarchFileWriter::WriteFileHeader()
{
	metaStream->Write((byte*)&fileHeader, DnarchFileHeader::HeaderSize);
}


void DnarchFileWriter::WriteFileFooter()
{
	uint32 blockCount = fileFooter.blockSizes.size();
	metaStream->Write((byte*)&blockCount, sizeof(uint32));
	metaStream->Write((byte*)fileFooter.blockSizes.data(), fileFooter.blockSizes.size() * sizeof(uint64));
}


DnarchFileReader::DnarchFileReader()
	:	metaStream(NULL)
	,	dataStream(NULL)
	,	blockIdx(0)
{}


DnarchFileReader::~DnarchFileReader()
{
	if (metaStream != NULL)
		delete metaStream;

	if (dataStream != NULL)
		delete dataStream;
}


void DnarchFileReader::StartDecompress(const std::string &fileName_, MinimizerParameters &minParams_)
{
	ASSERT(metaStream == NULL);

	metaStream = new FileStreamReader(fileName_ + ".cmeta");
	metaStream->SetBuffering(true);

	dataStream = new FileStreamReader(fileName_ + ".cdna");

	if (metaStream->Size() == 0 || dataStream->Size() == 0)
		throw Exception("Empty archive.");

	// Read file header
	//
	std::fill((uchar*)&fileHeader, (uchar*)&fileHeader + sizeof(DnarchFileHeader), 0);
	ReadFileHeader();

	if (fileHeader.footerOffset + (uint64)fileHeader.footerSize > metaStream->Size())
	{
		delete metaStream;
		metaStream = NULL;
		throw Exception("Corrupted archive.");
	}

	// clean footer
	//
	fileFooter.blockSizes.clear();

	metaStream->SetPosition(fileHeader.footerOffset);
	ReadFileFooter();

	metaStream->SetPosition(DnarchFileHeader::HeaderSize);

	minParams_ = fileHeader.minParams;
}


void DnarchFileReader::ReadFileHeader()
{
	metaStream->Read((byte*)&fileHeader, DnarchFileHeader::HeaderSize);
}


void DnarchFileReader::ReadFileFooter()
{
	uint32 blockCount = 0;
	metaStream->Read((byte*)&blockCount, sizeof(uint32));
	ASSERT(blockCount > 0);
	fileFooter.blockSizes.resize(blockCount);

	metaStream->Read((byte*)fileFooter.blockSizes.data(), fileFooter.blockSizes.size() * sizeof(uint64));
}


bool DnarchFileReader::ReadNextBin(CompressedDnaBlock *bin_)
{
	if (blockIdx >= fileFooter.blockSizes.size())
		return false;

	const uint64 bs = fileFooter.blockSizes[blockIdx];
	if (bin_->dataBuffer.data.Size() < bs)
		bin_->dataBuffer.data.Extend(bs + (bs / 8));

	dataStream->Read(bin_->dataBuffer.data.Pointer(), bs);
	bin_->dataBuffer.size = bs;

	blockIdx++;
	return true;
}


void DnarchFileReader::FinishDecompress()
{
	ASSERT(metaStream != NULL);
	ASSERT(dataStream != NULL);

	metaStream->Close();
	delete metaStream;
	metaStream = NULL;

	dataStream->Close();
	delete dataStream;
	dataStream = NULL;
}
