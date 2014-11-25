/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "Globals.h"

#include <vector>

#include "BinModule.h"
#include "FastqStream.h"
#include "DnaParser.h"
#include "DnaPacker.h"
#include "DnaCategorizer.h"
#include "BinFile.h"
#include "BinOperator.h"
#include "DnaBlockData.h"
#include "Exception.h"
#include "Thread.h"


void BinModule::Fastq2Bin(const std::vector<std::string> &inFastqFiles_, const std::string &outBinFile_, uint32 threadNum_,  bool compressedInput_)
{
	// TODO: try/catch to free resources
	//
	IFastqStreamReader* fastqFile = NULL;
	if (compressedInput_)
		fastqFile = new MultiFastqFileReaderGz(inFastqFiles_);
	else
		fastqFile = new MultiFastqFileReader(inFastqFiles_);


	BinFileWriter binFile;
	binFile.StartCompress(outBinFile_, config);

	const uint32 minimizersCount = config.minimizer.TotalMinimizersCount();
	if (threadNum_ > 1)
	{
		FastqChunkPool* fastqPool = NULL;
		FastqChunkQueue* fastqQueue = NULL;
		BinaryPartsPool* binPool = NULL;
		BinaryPartsQueue* binQueue = NULL;

		FastqChunkReader* fastqReader = NULL;
		BinChunkWriter* binWriter = NULL;

		const uint32 partNum = threadNum_ * 4;
		fastqPool = new FastqChunkPool(partNum, config.fastqBlockSize);
		fastqQueue = new FastqChunkQueue(partNum, 1);

		binPool = new BinaryPartsPool(partNum, minimizersCount);
		binQueue = new BinaryPartsQueue(partNum, threadNum_);

		fastqReader = new FastqChunkReader(fastqFile, fastqQueue, fastqPool);
		binWriter = new BinChunkWriter(&binFile, binQueue, binPool);

		// launch stuff
		//
		mt::thread readerThread(mt::ref(*fastqReader));

		std::vector<IOperator*> operators;
		operators.resize(threadNum_);

#ifdef USE_BOOST_THREAD
		boost::thread_group opThreadGroup;

		for (uint32 i = 0; i < threadNum_; ++i)
		{
			operators[i] = new BinEncoder(config.minimizer, config.catParams,
										  fastqQueue, fastqPool,
										  binQueue, binPool);
			opThreadGroup.create_thread(mt::ref(*operators[i]));
		}

		(*binWriter)();

		readerThread.join();
		opThreadGroup.join_all();


#else
		std::vector<mt::thread> opThreadGroup;

		for (uint32 i = 0; i < threadNum_; ++i)
		{
			operators[i] = new BinEncoder(config.minimizer, config.catParams,
										  fastqQueue, fastqPool, binQueue, binPool);
			opThreadGroup.push_back(mt::thread(mt::ref(*operators[i])));
		}

		(*binWriter)();

		readerThread.join();

		for (mt::thread& t : opThreadGroup)
		{
			t.join();
		}

#endif

		for (uint32 i = 0; i < threadNum_; ++i)
		{
			delete operators[i];
		}

		TFREE(binWriter);
		TFREE(fastqReader);

		TFREE(binQueue);
		TFREE(binPool);
		TFREE(fastqQueue);
		TFREE(fastqPool);
	}
	else
	{
		DnaParser parser;
		DnaCategorizer categorizer(config.minimizer, config.catParams);
		DnaPacker packer(config.minimizer);

		DataChunk fastqChunk(config.fastqBlockSize);
		std::vector<DnaRecord> records;
		records.resize(1 << 10);

		DnaBinBlock dnaBins(minimizersCount);
		BinaryBinBlock binBins;
		DataChunk dnaBuffer;

		while (fastqFile->ReadNextChunk(&fastqChunk))
		{
			uint64 recordsCount = 0;
			parser.ParseFrom(fastqChunk, dnaBuffer, records, recordsCount);

			ASSERT(recordsCount > 0);
			categorizer.Categorize(records, recordsCount, dnaBins);

			packer.PackToBins(dnaBins, binBins);

			binFile.WriteNextBlock(&binBins);
		}
	}

	binFile.FinishCompress();

	delete fastqFile;
}


void BinModule::Bin2Dna(const std::string &inBinFile_, const std::string &outDnaFile_)
{
	// TODO: try/catch to free resources
	//
	BinFileReader binFile;

	binFile.StartDecompress(inBinFile_, config);
	uint32 minimizersCount = config.minimizer.TotalMinimizersCount();

	DnaFileWriter dnaFile(outDnaFile_);
	DataChunk fastqChunk(config.fastqBlockSize >> 1);			// WARNING! --- here can be a BUG
	DnaPacker packer(config.minimizer);
	DnaParser parser;

	DnaBinBlock dnaBins(minimizersCount);
	BinaryBinBlock binBins;
	DataChunk dnaBuffer;

	while (binFile.ReadNextBlock(&binBins))
	{
		packer.UnpackFromBins(binBins, dnaBins, dnaBuffer);
		parser.ParseTo(dnaBins, fastqChunk);

		dnaFile.WriteNextChunk(&fastqChunk);
	}

	dnaFile.Close();
	binFile.FinishDecompress();
}
