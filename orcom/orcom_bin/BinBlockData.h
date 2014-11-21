/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_BINBLOCKDATA
#define H_BINBLOCKDATA

#include "Globals.h"

#include <vector>
#include <map>

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
	typedef std::map<uint32, BinaryBinDescriptor> DescriptorMap;

	static const uint64 DefaultMetaBufferSize = 1 << 6;
	static const uint64 DefaultDnaBufferSize = 1 << 8;

	DescriptorMap descriptors;
	Buffer metaData;
	Buffer dnaData;

	uint64 metaSize;
	uint64 dnaSize;
	uint64 rawDnaSize;

	BinaryBinBlock(uint64 dnaBufferSize_ = DefaultDnaBufferSize, uint64 metaBufferSize_ = DefaultMetaBufferSize)
		:	metaData(metaBufferSize_)
		,	dnaData(dnaBufferSize_)
	{
		Clear();
	}

	void Clear()
	{
		metaSize = 0;
		dnaSize = 0;
		rawDnaSize = 0;

		descriptors.clear();		// CHECKME!!!
	}

	void Reset()		// for compatibility with queues
	{
		Clear();
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

#endif // H_BINBLOCKDATA
