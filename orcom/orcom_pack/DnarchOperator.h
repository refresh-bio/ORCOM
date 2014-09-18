#ifndef DNARCHOPERATOR_H
#define DNARCHOPERATOR_H

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/DataPool.h"
#include "../orcom_bin/DataQueue.h"

#include "Params.h"
#include "CompressedBlockData.h"
#include "BinFileExtractor.h"
#include "DnarchFile.h"


// operators for multi threaded processing
//

struct MinimizerBinPart : public BinaryBinBlock
{
	uint32 minimizer;

	MinimizerBinPart(uint64 dnaBufferSize_ = 1 << 20, uint64 metaBufferSize_ = 1 << 16)
		:	BinaryBinBlock(dnaBufferSize_, metaBufferSize_)
		,	minimizer(0)
	{}

	void Reset()
	{
		minimizer = 0;

		metaSize = 0;
		dnaSize = 0;
	}
};


typedef TDataPool<MinimizerBinPart> MinimizerPartsPool;
typedef TDataPool<CompressedDnaBlock> CompressedDnaPartsPool;

typedef TDataQueue<MinimizerBinPart> MinimizerPartsQueue;
typedef TDataQueue<CompressedDnaBlock> CompressedDnaPartsQueue;

typedef DataChunk RawDnaPart;
typedef TDataPool<RawDnaPart> RawDnaPartsPool;
typedef TDataQueue<RawDnaPart> RawDnaPartsQueue;


class BinPartsExtractor : public IOperator
{
public:
	BinPartsExtractor(BinFileExtractor* partsStream_, MinimizerPartsQueue* partsQueue_, MinimizerPartsPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	BinFileExtractor* partsStream;
	MinimizerPartsQueue* partsQueue;
	MinimizerPartsPool* partsPool;
};

class BinPartsCompressor : public IOperator
{
public:
	BinPartsCompressor(const MinimizerParameters& minimizer_, const CompressorParams& params_,
					   MinimizerPartsQueue* inPartsQueue_, MinimizerPartsPool* inPartsPool_,
					   CompressedDnaPartsQueue* outPartsQueue_, CompressedDnaPartsPool* outPartsPool_)
		:	minimizer(minimizer_)
		,	params(params_)
		,	inPartsQueue(inPartsQueue_)
		,	inPartsPool(inPartsPool_)
		,	outPartsQueue(outPartsQueue_)
		,	outPartsPool(outPartsPool_)
	{}

	void Run();

protected:
	const MinimizerParameters minimizer;
	const CompressorParams params;

	MinimizerPartsQueue* inPartsQueue;
	MinimizerPartsPool* inPartsPool;
	CompressedDnaPartsQueue* outPartsQueue;
	CompressedDnaPartsPool* outPartsPool;
};

class DnarchPartsWriter : public IOperator
{
public:
	DnarchPartsWriter(DnarchFileWriter* partsStream_, CompressedDnaPartsQueue* partsQueue_, CompressedDnaPartsPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	DnarchFileWriter* partsStream;
	CompressedDnaPartsQueue* partsQueue;
	CompressedDnaPartsPool* partsPool;
};





class DnarchPartsReader : public IOperator
{
public:
	DnarchPartsReader(DnarchFileReader* partsStream_, CompressedDnaPartsQueue* partsQueue_, CompressedDnaPartsPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	typedef CompressedDnaBlock PartType;

	DnarchFileReader* partsStream;
	CompressedDnaPartsQueue* partsQueue;
	CompressedDnaPartsPool* partsPool;
};

class DnaPartsDecompressor : public IOperator
{
public:
	DnaPartsDecompressor(const MinimizerParameters& minimizer_,
						 CompressedDnaPartsQueue* inPartsQueue_, CompressedDnaPartsPool* inPartsPool_,
						 RawDnaPartsQueue* outPartsQueue_, RawDnaPartsPool* outPartsPool_)
		:	minimizer(minimizer_)
		,	inPartsQueue(inPartsQueue_)
		,	inPartsPool(inPartsPool_)
		,	outPartsQueue(outPartsQueue_)
		,	outPartsPool(outPartsPool_)
	{}

	void Run();

protected:
	typedef CompressedDnaBlock InPartType;
	typedef RawDnaPart OutPartType;

	const MinimizerParameters minimizer;

	CompressedDnaPartsQueue* inPartsQueue;
	CompressedDnaPartsPool* inPartsPool;
	RawDnaPartsQueue* outPartsQueue;
	RawDnaPartsPool* outPartsPool;
};

class RawDnaPartsWriter : public IOperator
{
public:
	RawDnaPartsWriter(FileStreamWriter* partsStream_, RawDnaPartsQueue* partsQueue_, RawDnaPartsPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	typedef RawDnaPart PartType;

	FileStreamWriter* partsStream;
	RawDnaPartsQueue* partsQueue;
	RawDnaPartsPool* partsPool;
};


#endif // DNARCHOPERATOR_H
