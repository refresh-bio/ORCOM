#ifndef BINOPERATOR_H
#define BINOPERATOR_H

#include "Globals.h"
#include "DataPool.h"
#include "DataQueue.h"
#include "BinBlockData.h"
#include "Params.h"

typedef BinaryBinBlock BinaryPart;

// operators for multi threaded processing
//
typedef TDataPool<BinaryPart> BinaryPartsPool;
typedef TDataPool<DataChunk> FastqChunkPool;

typedef TDataQueue<BinaryPart> BinaryPartsQueue;
typedef TDataQueue<DataChunk> FastqChunkQueue;


// readers and writers can be fully templatized !!!
//
class FastqChunkReader : public IOperator
{
public:
	FastqChunkReader(IFastqStreamReader* partsStream_, FastqChunkQueue* partsQueue_, FastqChunkPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	IFastqStreamReader* partsStream;
	FastqChunkQueue* partsQueue;
	FastqChunkPool* partsPool;
};

class BinChunkWriter : public IOperator
{
public:
	BinChunkWriter(BinFileWriter* partsStream_, BinaryPartsQueue* partsQueue_, BinaryPartsPool* partsPool_)
		:	partsStream(partsStream_)
		,	partsQueue(partsQueue_)
		,	partsPool(partsPool_)
	{}

	void Run();

private:
	BinFileWriter* partsStream;
	BinaryPartsQueue* partsQueue;
	BinaryPartsPool* partsPool;
};
//
//
// ///////


class BinEncoder : public IOperator
{
public:
	BinEncoder(const MinimizerParameters& params_,
			   const CategorizerParameters& catParams_,
			  FastqChunkQueue* fqPartsQueue_, FastqChunkPool* fqPartsPool_,
			  BinaryPartsQueue* binPartsQueue_, BinaryPartsPool* binPartsPool_)
		:	params(params_)
		,	catParams(catParams_)
		,	fqPartsQueue(fqPartsQueue_)
		,	fqPartsPool(fqPartsPool_)
		,	binPartsQueue(binPartsQueue_)
		,	binPartsPool(binPartsPool_)
	{}

protected:
	const MinimizerParameters params;
	const CategorizerParameters catParams;

	FastqChunkQueue* fqPartsQueue;
	FastqChunkPool* fqPartsPool;
	BinaryPartsQueue* binPartsQueue;
	BinaryPartsPool* binPartsPool;


	void Run();
};



#endif // BINOPERATOR_H
