#ifndef BINFILEEXTRACTOR_H
#define BINFILEEXTRACTOR_H

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/BinBlockData.h"
#include "../orcom_bin/BinFile.h"


class BinFileExtractor : public BinFileReader
{
public:
	static const uint32 DefaultMinimumBinSize = 64;


	struct SubBlockDescriptor : public BinaryBinDescriptor
	{
		uint64 metaPosition;
		uint64 dnaPosition;

		SubBlockDescriptor()
			:	metaPosition(0)
			,	dnaPosition(0)
		{}
	};

	struct BlockDescriptor : public TBinaryBinDescriptor<uint64>
	{
		uint32 signature;
		std::vector<SubBlockDescriptor> subBlocks;

		BlockDescriptor()
			:	signature(0)
		{}
	};


	BinFileExtractor(uint32 minBinSize_ = DefaultMinimumBinSize);
	~BinFileExtractor();

	void StartDecompress(const std::string& fileName_, BinModuleConfig& params_);

	bool ExtractNextSmallBin(BinaryBinBlock& bin_, uint32& minimizerId_);
	bool ExtractNextStdBin(BinaryBinBlock& bin_, uint32& minimizerId_);
	bool ExtractNBin(BinaryBinBlock& bin_, uint32& minimizerId_);

	uint64 BlockCount() const
	{
		return fileHeader.blockCount;
	}

	// TODO: optimize
	std::vector<const BlockDescriptor*> GetSmallBlockDescriptors() const
	{
		ASSERT(blockDescriptors.size() > 0);

		std::vector<const BlockDescriptor*> desc;
		for (uint32 i = stdBlockCount; i < blockDescriptors.size(); ++i)	// TODO: just use insert / back_inserter
		{
			const BlockDescriptor* bd = &blockDescriptors[i];
			ASSERT(bd->metaSize > 0 && bd->recordsCount < minBinSize);
			desc.push_back(bd);
		}

		return desc;
	}

	// TODO: optimize
	std::vector<const BlockDescriptor*> GetStdBlockDescriptors() const
	{
		ASSERT(blockDescriptors.size() > 0);

		std::vector<const BlockDescriptor*> desc;
		for (uint32 i = 0; i < stdBlockCount; ++i)							// TODO: just use insert / back_inserter
		{
			const BlockDescriptor* bd = &blockDescriptors[i];
			ASSERT(bd->metaSize > 0 && bd->recordsCount >= minBinSize);
			desc.push_back(bd);
		}
		return desc;
	}

	const BlockDescriptor* GetNBlockDescriptor() const
	{
		return &nBlockDescriptor;
	}

private:

	using BinFileReader::ReadNextBlock;

	const uint32 minBinSize;

	uint64 currentSmallBlockIdx;
	uint64 currentStdBlockIdx;
	uint64 stdBlockCount;

	std::vector<BlockDescriptor> blockDescriptors;
	BlockDescriptor nBlockDescriptor;

	void ExtractNextBin(const BlockDescriptor& desc_, BinaryBinBlock &bin_);
};

#endif // BINFILEEXTRACTOR_H
