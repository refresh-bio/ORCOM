/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_COMPRESSEDBLOCKDATA
#define H_COMPRESSEDBLOCKDATA

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/Buffer.h"
#include "../orcom_bin/Collections.h"


struct DnaCompressedBin
{
	enum BufferNames
	{
		FlagBuffer = 0,
		LetterXBuffer,
		RevBuffer,

		HardReadsBuffer,
		LzIdBuffer,
		ShiftBuffer,
		LenBuffer,
		MatchBuffer,

		PPMdStartBuffer = HardReadsBuffer,
		PPMdEndBuffer = MatchBuffer
	};

	static const uint32 BuffersNum = PPMdEndBuffer + 1;
	static const uint64 DefaultBufferSize = 1 << 20;

	DataChunk* buffers[BuffersNum];
	uint64 recordsCount;

	DnaCompressedBin(uint64 bufferSize_ = DefaultBufferSize)
		:	recordsCount(0)
	{
		for (uint32 i = 0; i < BuffersNum; ++i)
			buffers[i] = new DataChunk(bufferSize_);
	}

	virtual ~DnaCompressedBin()
	{
		for (uint32 i = 0; i < BuffersNum; ++i)
			delete buffers[i];
	}

	void Clear()
	{
		for (uint32 i = 0; i < BuffersNum; ++i)
			buffers[i]->size = 0;
		recordsCount = 0;
	}
};


struct WorkBuffers
{
	DnaBin dnaBin;
	DnaCompressedBin dnaWorkBin;
	DataChunk dnaBuffer;

	void Clear()
	{
		dnaBin.Clear();
		dnaWorkBin.Clear();
		dnaBuffer.size = 0;
	}
};


struct CompressedDnaBlock
{
	static const uint32 BuffersCount = DnaCompressedBin::BuffersNum;

	uint32 signatureId;
	DataChunk dataBuffer;
	WorkBuffers workBuffers;

	uint64 bufferSizes[BuffersCount];		// only for diagnostics purposes

	CompressedDnaBlock(uint64 bufferSize_ = DataChunk::DefaultBufferSize)
		:	signatureId(0)
		,	dataBuffer(bufferSize_)
	{
		std::fill(bufferSizes, bufferSizes + BuffersCount, 0);
	}

	void Reset()
	{
		std::fill(bufferSizes, bufferSizes + BuffersCount, 0);

		dataBuffer.size = 0;
		workBuffers.Clear();
	}
};


#endif // H_COMPRESSEDBLOCKDATA
