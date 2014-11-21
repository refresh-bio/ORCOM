/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "../orcom_bin/Globals.h"

#include "BinFileExtractor.h"
#include "../orcom_bin/Exception.h"


#if (DEV_DEBUG_PRINT)
#	include <iostream>
#endif

struct BlockDescriptorComparator
{
	bool operator() (const BinFileExtractor::BlockDescriptor& b1_, const BinFileExtractor::BlockDescriptor& b2_)
	{
		return b1_.rawDnaSize > b2_.rawDnaSize;
	}
};


BinFileExtractor::BinFileExtractor(uint32 minBinSize_)
	:	minBinSize(minBinSize_)
	,	currentSmallBlockIdx(0)
	,	currentStdBlockIdx(0)
	,	stdBlockCount(0)
{}


void BinFileExtractor::StartDecompress(const std::string &fileName_, BinModuleConfig &params_)
{
	ASSERT(metaStream == NULL);
	ASSERT(dnaStream == NULL);

	metaStream = new FileStreamReader(fileName_ + ".bmeta");
	dnaStream = new FileStreamReader(fileName_ + ".bdna");

	if (metaStream->Size() == 0)
		throw Exception("Empty file.");

	// read header and footer
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

	fileFooter.Clear();

	metaStream->SetPosition(fileHeader.footerOffset);
	ReadFileFooter();
	params_ = fileFooter.params;

	metaStream->SetPosition(BinFileHeader::HeaderSize);

	uint64 fileMetaPos = BinFileHeader::HeaderSize;
	uint64 fileDnaPos = 0;


	// prepare block descriptors
	//
	std::map<uint32, BlockDescriptor> descriptorMap;

	const uint64 binsPerBlock = fileFooter.params.minimizer.TotalMinimizersCount();
	nBlockDescriptor.signature = binsPerBlock;

	currentNonEmptyBlockId = 0;		// we can try to reuse this
	currentBlockId = 0;				// resue this also

	currentSmallBlockIdx = 0;
	currentStdBlockIdx = 0;


	// read block descriptors
	//
	uint64 rawDnaStreamSize = 0;

	uint64 totalDnaSize = 0;
	uint64 totalRawSize = 0;

	for (uint64 i = 0; i < fileFooter.metaBinSizes.size(); ++i)
	{
		const uint64 metaSize = fileFooter.metaBinSizes[i];

		if (metaSize == 0)
			continue;

		const uint64 minId = i % (binsPerBlock + 1);

		ASSERT(currentNonEmptyBlockId < fileFooter.dnaBinSizes.size());

		SubBlockDescriptor subDesc;
		subDesc.metaSize = metaSize;
		subDesc.dnaSize = fileFooter.dnaBinSizes[currentNonEmptyBlockId];
		subDesc.recordsCount = fileFooter.recordsCounts[currentNonEmptyBlockId];
		subDesc.rawDnaSize = fileFooter.rawDnaSizes[currentNonEmptyBlockId];
		subDesc.metaPosition = fileMetaPos;
		subDesc.dnaPosition  = fileDnaPos;

		ASSERT(subDesc.metaSize < (1<<30));
		ASSERT(subDesc.dnaSize < (1<<30));

		// shall 32 bits be enough here?
		BlockDescriptor* blockDesc;
		if (minId != binsPerBlock)
			blockDesc = &descriptorMap[minId];
		else
			blockDesc = &nBlockDescriptor;

		blockDesc->metaSize += subDesc.metaSize;
		blockDesc->dnaSize += subDesc.dnaSize;
		blockDesc->recordsCount += subDesc.recordsCount;
		blockDesc->rawDnaSize += subDesc.rawDnaSize;
		blockDesc->subBlocks.push_back(subDesc);

		fileMetaPos += subDesc.metaSize;
		fileDnaPos += subDesc.dnaSize;

		rawDnaStreamSize += subDesc.rawDnaSize;

		currentNonEmptyBlockId++;

		totalDnaSize += subDesc.dnaSize;
		totalRawSize += subDesc.rawDnaSize;
	}


	// free the footer memory
	//
	{
		std::vector<uint32> t1, t2, t3, t4;
		fileFooter.metaBinSizes.swap(t1);
		fileFooter.dnaBinSizes.swap(t2);
		fileFooter.recordsCounts.swap(t3);
		fileFooter.rawDnaSizes.swap(t4);
	}

	// convert descriptor map to internal vector of block descriptors and swap the sub blocks
	//
	{
		blockDescriptors.resize(descriptorMap.size());
		uint32 descId = 0;
		for (std::map<uint32, BlockDescriptor>::iterator i = descriptorMap.begin();
			 i != descriptorMap.end(); ++i)
		{
			const uint32 signature = i->first;
			BlockDescriptor& mapDesc = i->second;
			BlockDescriptor& desc = blockDescriptors[descId++];
			desc.signature = signature;
			desc.dnaSize = mapDesc.dnaSize;
			desc.metaSize = mapDesc.metaSize;
			desc.rawDnaSize = mapDesc.rawDnaSize;
			desc.recordsCount = mapDesc.recordsCount;
			desc.subBlocks.swap(mapDesc.subBlocks);
		}
	}


	// sort descriptors by size
	//
	std::sort(blockDescriptors.begin(), blockDescriptors.end(), BlockDescriptorComparator());


#if (DEV_DEBUG_PRINT)
	std::cerr << "--------.--------*--------.--------\n";
	std::cerr << "Descriptors count: " << blockDescriptors.size() << '\n';
	std::cerr << "Minimum bin size: " << minBinSize << '\n';
	std::cerr << "Bin sizes:\n";

	for (uint64 i = 0; i < blockDescriptors.size(); ++i)
	{
		const BlockDescriptor& desc = blockDescriptors[i];
		std::cerr << "[ " << desc.signature << " ] " << desc.recordsCount << '\n';
	}
	std::cerr << "[ " << nBlockDescriptor.signature << " ] " << nBlockDescriptor.recordsCount << std::endl;
#endif


	// partition
	//
	const uint64 dnaThresholdSize = rawDnaStreamSize * 0.8;
	uint64 dnaCulmSize = 0;
	uint64 validBins = 0;
	for ( ; validBins < blockDescriptors.size() && dnaCulmSize < dnaThresholdSize; validBins++)
	{
		ASSERT(blockDescriptors[validBins].recordsCount >= minBinSize);
		dnaCulmSize += blockDescriptors[validBins].rawDnaSize;
	}

	std::random_shuffle(blockDescriptors.begin(), blockDescriptors.begin() + validBins);

	for (currentSmallBlockIdx = validBins; currentSmallBlockIdx < blockDescriptors.size(); currentSmallBlockIdx++)
	{
		if (blockDescriptors[currentSmallBlockIdx].recordsCount < minBinSize || blockDescriptors[currentSmallBlockIdx].recordsCount == 0)
			break;
	}
	stdBlockCount = currentSmallBlockIdx;


	// remove the empty descriptors
	//
	for (validBins = currentSmallBlockIdx; validBins < blockDescriptors.size(); validBins++)
	{
		if (blockDescriptors[validBins].recordsCount == 0)
		{
			blockDescriptors.resize(validBins);
			break;
		}
	}
}


bool BinFileExtractor::ExtractNextStdBin(BinaryBinBlock &bin_, uint32 &minimizerId_)
{
	bin_.Clear();
	bin_.descriptors.clear();

	// have we reached N bin?
	if (currentStdBlockIdx >= stdBlockCount)
	{
		minimizerId_ = (uint32)-1;
		return false;
	}

	const BlockDescriptor& blockDesc = blockDescriptors[currentStdBlockIdx];
	ASSERT(blockDesc.metaSize > 0 && blockDesc.recordsCount >= minBinSize);

	ExtractNextBin(blockDesc, bin_);
	minimizerId_ = blockDesc.signature;

	currentStdBlockIdx++;
	return true;
}


bool BinFileExtractor::ExtractNextSmallBin(BinaryBinBlock &bin_, uint32 &minimizerId_)
{
	bin_.Clear();
	bin_.descriptors.clear();

	// have we reached N bin?
	if (currentSmallBlockIdx >= blockDescriptors.size() || blockDescriptors[currentSmallBlockIdx].metaSize == 0)
	{
		minimizerId_ = (uint32)-1;
		return false;
	}

	const BlockDescriptor& blockDesc = blockDescriptors[currentSmallBlockIdx];
	ASSERT(blockDesc.recordsCount < minBinSize);

	ExtractNextBin(blockDesc, bin_);
	minimizerId_ = blockDesc.signature;

	currentSmallBlockIdx++;
	return true;
}


bool BinFileExtractor::ExtractNBin(BinaryBinBlock &bin_, uint32& minimizerId_)
{
	bin_.Clear();
	bin_.descriptors.clear();

	ExtractNextBin(nBlockDescriptor, bin_);
	minimizerId_ = nBlockDescriptor.signature;

	return true;
}


void BinFileExtractor::ExtractNextBin(const BlockDescriptor &desc_, BinaryBinBlock &bin_)
{
	ASSERT(desc_.recordsCount > 0);

	if (bin_.metaData.Size() < desc_.metaSize)
		bin_.metaData.Extend(desc_.metaSize);

	if (bin_.dnaData.Size() < desc_.dnaSize)
		bin_.dnaData.Extend(desc_.dnaSize);

	for (uint64 i = 0; i < desc_.subBlocks.size(); ++i)
	{
		const SubBlockDescriptor& subBlock = desc_.subBlocks[i];
		ASSERT(subBlock.metaPosition != 0);
		ASSERT(subBlock.metaSize != 0);

		metaStream->SetPosition(subBlock.metaPosition);
		dnaStream->SetPosition(subBlock.dnaPosition);

		metaStream->Read(bin_.metaData.Pointer() + bin_.metaSize, subBlock.metaSize);
		dnaStream->Read(bin_.dnaData.Pointer() + bin_.dnaSize, subBlock.dnaSize);

		bin_.metaSize += subBlock.metaSize;
		bin_.dnaSize += subBlock.dnaSize;
		bin_.rawDnaSize += subBlock.rawDnaSize;

		// here we need to insert descriptors into the map treating it as a vector...
		bin_.descriptors.insert(std::make_pair(bin_.descriptors.size(), subBlock));
	}
}
