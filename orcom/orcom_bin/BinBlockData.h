#ifndef BINBLOCKDATA_H
#define BINBLOCKDATA_H

#include "Globals.h"

#include <vector>

#include "Buffer.h"
#include "Collections.h"


template <typename _TSizeType>
struct TBinaryBinDescriptor
{
	_TSizeType metaSize;
	_TSizeType dnaSize;

	_TSizeType recordsCount;
	_TSizeType rawDnaSize;

	TBinaryBinDescriptor()
	{
		Clear();
	}

	void Clear()
	{
		metaSize = 0;
		dnaSize = 0;
		recordsCount = 0;
		rawDnaSize = 0;
	}
};

typedef TBinaryBinDescriptor<uint32> BinaryBinDescriptor;


struct BinaryBinBlock
{
	static const uint64 DefaultMetaBufferSize = 1 << 6;
	static const uint64 DefaultDnaBufferSize = 1 << 8;

	std::vector<BinaryBinDescriptor> descriptors;
	Buffer metaData;
	Buffer dnaData;

	uint64 metaSize;
	uint64 dnaSize;
	uint64 rawDnaSize;

	BinaryBinBlock(uint64 dnaBufferSize_ = DefaultDnaBufferSize, uint64 metaBufferSize_ = DefaultMetaBufferSize)
		:	metaData(metaBufferSize_)
		,	dnaData(dnaBufferSize_)
	{
		Reset();
	}

	void Reset()
	{
		metaSize = 0;
		dnaSize = 0;
		rawDnaSize = 0;
	}
};

struct BinaryBin
{
	static const uint64 DefaultMetaBufferSize = 1 << 6;
	static const uint64 DefaultDnaBufferSize = 1 << 8;

	Buffer metaData;
	uint64 metaSize;

	Buffer dnaData;
	uint64 dnaSize;
	uint64 recordsCount;

	BinaryBin(uint64 dnaBufferSize_ = DefaultDnaBufferSize, uint64 metaBufferSize_ = DefaultMetaBufferSize)
		:	metaData(metaBufferSize_)
		,	metaSize(0)
		,	dnaData(dnaBufferSize_)
		,	dnaSize(0)
		,	recordsCount(0)
	{
		ASSERT(dnaBufferSize_ > 0);
		ASSERT(metaBufferSize_ > 0);
	}

	void Reset()
	{
		metaSize = 0;
		dnaSize = 0;
		recordsCount = 0;
	}
};


#endif // BINBLOCKDATA_H
