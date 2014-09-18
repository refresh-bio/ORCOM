#include "../orcom_bin/Globals.h"

#if (DEV_PRINT_STATS)
#	include <iostream>
#endif

#include "DnarchModule.h"
#include "BinFileExtractor.h"
#include "DnarchFile.h"
#include "DnaCompressor.h"
#include "DnarchOperator.h"
#include "Params.h"

#include "../orcom_bin/DnaPacker.h"
#include "../orcom_bin/DnaParser.h"
#include "../orcom_bin/Thread.h"


void DnarchModule::Bin2Dnarch(const std::string &inBinFile_, const std::string &outDnarchFile_, const CompressorParams& params_, uint32 threadsNum_)
{
	BinModuleConfig conf;
	BinFileExtractor* extractor = new BinFileExtractor(params_.minBinSize);

	extractor->StartDecompress(inBinFile_, conf);

	DnarchFileWriter* dnarch = new DnarchFileWriter();
	dnarch->StartCompress(outDnarchFile_, conf.minimizer, params_);

	if (threadsNum_ > 1)
	{
		const uint32 partNum = threadsNum_ + (threadsNum_ >> 1);//threadsNum_ * 2;
		const uint64 dnaBufferSize = 1 << 20;
		const uint64 outBufferSize = 1 << 20;

		MinimizerPartsPool* inPool = new MinimizerPartsPool(partNum, dnaBufferSize);
		MinimizerPartsQueue* inQueue = new MinimizerPartsQueue(partNum, 1);

		CompressedDnaPartsPool* outPool = new CompressedDnaPartsPool(partNum, outBufferSize);
		CompressedDnaPartsQueue* outQueue = new CompressedDnaPartsQueue(partNum, threadsNum_);

		BinPartsExtractor* inReader = new BinPartsExtractor(extractor, inQueue, inPool);
		DnarchPartsWriter* outWriter = new DnarchPartsWriter(dnarch, outQueue, outPool);


		// preprocess small bins and N bin <-- this should be done internally
		//
		{
			DnaCompressor compressor(conf.minimizer, params_);
			DnaPacker packer(conf.minimizer);

			uint32 signatureId = 0;

			CompressedDnaBlock compBin;
			BinaryBinBlock binBin;

			std::vector<const BinFileExtractor::BlockDescriptor*> descriptors = extractor->GetSmallBlockDescriptors();

			uint64 totalDnaBufferSize = 0;
			uint64 totalRecords = 0;
			for (uint32 i = 0; i < descriptors.size(); ++i)
			{
				totalDnaBufferSize += descriptors[i]->rawDnaSize;
				totalRecords += descriptors[i]->recordsCount;
			}
			const BinFileExtractor::BlockDescriptor* nd = extractor->GetNBlockDescriptor();
			totalDnaBufferSize += nd->rawDnaSize;
			totalRecords += nd->recordsCount;

			if (compBin.workBuffers.dnaBuffer.data.Size() < totalDnaBufferSize)
				compBin.workBuffers.dnaBuffer.data.Extend(totalDnaBufferSize);


			// extract and unpack small bins
			//
			while (extractor->ExtractNextSmallBin(binBin, signatureId))
			{
				ASSERT(binBin.metaSize != 0);

				packer.UnpackFromBin(binBin, compBin.workBuffers.dnaBin, signatureId, compBin.workBuffers.dnaBuffer, true);
			}

			if (extractor->ExtractNBin(binBin, signatureId) && binBin.metaSize > 0)
				packer.UnpackFromBin(binBin, compBin.workBuffers.dnaBin, signatureId, compBin.workBuffers.dnaBuffer, true);


			// un-reverse-compliment records
			//
			{
				char rcBuf[DnaRecord::MaxDnaLen];
				DnaRecord rcRec;
				rcRec.dna = rcBuf;
				rcRec.reverse = true;

				for (uint64 i = 0; i < compBin.workBuffers.dnaBin.Size(); ++i)
				{
					DnaRecord& r = compBin.workBuffers.dnaBin[i];
					if (r.reverse)
					{
						r.ComputeRC(rcRec);
						std::copy(rcRec.dna, rcRec.dna + r.len, r.dna);
						r.reverse = false;
						r.minimizerPos = 0;
					}
				}
			}

			// compress all bins together
			//
			const uint32 nSignature = conf.minimizer.TotalMinimizersCount();
			compressor.CompressDna(compBin.workBuffers.dnaBin, nSignature, totalDnaBufferSize, compBin.workBuffers.dnaWorkBin, compBin);

			dnarch->WriteNextBin(&compBin);
		}



		// launch stuff
		//
		mt::thread readerThread(mt::ref(*inReader));

		std::vector<IOperator*> operators;
		operators.resize(threadsNum_);

#ifdef USE_BOOST_THREAD
		boost::thread_group opThreadGroup;

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			operators[i] = new BinPartsCompressor(conf.minimizer, params_, 
												  inQueue, inPool,
												  outQueue, outPool);
			opThreadGroup.create_thread(mt::ref(*operators[i]));
		}

		(*outWriter)();

		readerThread.join();
		opThreadGroup.join_all();


#else
		std::vector<th::thread> opThreadGroup;

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			operators[i] = new BinPartsCompressor(conf.minimizer, params_,
												  inQueue, inPool,
												  outQueue, outPool,
												  metaMapper, dnaMapper);
			opThreadGroup.push_back(th::thread(mt::ref(*operators[i])));
		}

		(*outWriter)();

		readerThread.join();

		for (th::thread& t : opThreadGroup)
		{
			t.join();
		}

#endif

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			delete operators[i];
		}

		TFREE(outWriter);
		TFREE(inReader);

		TFREE(outQueue);
		TFREE(outPool);
		TFREE(inQueue);
		TFREE(inPool);
	}
	else
	{
		DnaCompressor compressor(conf.minimizer, params_);
		DnaPacker packer(conf.minimizer);

		uint32 minId = 0;

		CompressedDnaBlock compBin;
		BinaryBinBlock binBin;

		// preprocess small bins and N bin <--- this should be done internally
		//
		{
			std::vector<const BinFileExtractor::BlockDescriptor*> descriptors = extractor->GetSmallBlockDescriptors();

			uint64 totalDnaBufferSize = 0;
			uint64 totalRecords = 0;
			for (uint32 i = 0; i < descriptors.size(); ++i)
			{
				totalDnaBufferSize += descriptors[i]->rawDnaSize;
				totalRecords += descriptors[i]->recordsCount;
			}
			const BinFileExtractor::BlockDescriptor* nd = extractor->GetNBlockDescriptor();
			totalDnaBufferSize += nd->rawDnaSize;
			totalRecords += nd->recordsCount;

			if (compBin.workBuffers.dnaBuffer.data.Size() < totalDnaBufferSize)
				compBin.workBuffers.dnaBuffer.data.Extend(totalDnaBufferSize);

			// extract and unpack small bins
			//
			while (extractor->ExtractNextSmallBin(binBin, minId))
			{
				if (binBin.metaSize == 0)
					continue;

				packer.UnpackFromBin(binBin, compBin.workBuffers.dnaBin, minId, compBin.workBuffers.dnaBuffer, true);
			}

			// un-reverse-compliment records
			//
			{
				char rcBuf[DnaRecord::MaxDnaLen];
				DnaRecord rcRec;
				rcRec.dna = rcBuf;
				rcRec.reverse = true;

				for (uint64 i = 0; i < compBin.workBuffers.dnaBin.Size(); ++i)
				{
					DnaRecord& r = compBin.workBuffers.dnaBin[i];
					if (r.reverse)
					{
						r.ComputeRC(rcRec);
						std::copy(rcRec.dna, rcRec.dna + r.len, r.dna);
						r.reverse = false;
						r.minimizerPos = 0;
					}
				}
			}

			// extract and unpack the last one
			//
			uint32 nSignature = 0;
			if (extractor->ExtractNBin(binBin, nSignature) && binBin.metaSize > 0)
				packer.UnpackFromBin(binBin, compBin.workBuffers.dnaBin, nSignature, compBin.workBuffers.dnaBuffer, true);

			// compress them all together
			//
			compressor.CompressDna(compBin.workBuffers.dnaBin, nSignature, totalDnaBufferSize, compBin.workBuffers.dnaWorkBin, compBin);

			dnarch->WriteNextBin(&compBin);
		}

		// process std bins
		//
		while (extractor->ExtractNextStdBin(binBin, minId))
		{
			if (binBin.metaSize == 0)
				continue;

			packer.UnpackFromBin(binBin, compBin.workBuffers.dnaBin, minId, compBin.workBuffers.dnaBuffer);

			ASSERT(binBin.rawDnaSize > 0);
			compressor.CompressDna(compBin.workBuffers.dnaBin, minId, binBin.rawDnaSize, compBin.workBuffers.dnaWorkBin, compBin);

			dnarch->WriteNextBin(&compBin);
		}
	}

	extractor->FinishDecompress();
	dnarch->FinishCompress();

#if (DEV_PRINT_STATS)
	// TODO: move this outside
	std::vector<uint64> ss = dnarch->GetStreamSizes();
	std::cerr << "Stream sizes:\n[stream_id] : stream_size\n";
	for (uint32 i = 0; i < ss.size(); ++i)
	{
		std::cerr << '[' << i << "] : " << ss[i] << '\n';
	}
	std::cerr << std::endl;
#endif

	delete dnarch;
	delete extractor;
}



void DnarchModule::Dnarch2Dna(const std::string &inDnarchFile_, const std::string &outDnaFile_, uint32 threadsNum_)
{
	DnarchFileReader* dnarch = new DnarchFileReader();
	MinimizerParameters minParams;

	dnarch->StartDecompress(inDnarchFile_, minParams);
	FileStreamWriter* dnaFile = new FileStreamWriter(outDnaFile_);

	if (threadsNum_ > 1)
	{
		const uint32 partNum = threadsNum_ * 2;
		const uint64 inBufferSize = 1 << 25;
		const uint64 outBufferSize = 1 << 23;

		CompressedDnaPartsPool* inPool = new CompressedDnaPartsPool(partNum, inBufferSize);
		CompressedDnaPartsQueue* inQueue = new CompressedDnaPartsQueue(partNum, 1);

		RawDnaPartsPool* outPool = new RawDnaPartsPool(partNum, outBufferSize);
		RawDnaPartsQueue* outQueue = new RawDnaPartsQueue(partNum, threadsNum_);

		DnarchPartsReader* inReader = new DnarchPartsReader(dnarch, inQueue, inPool);
		RawDnaPartsWriter* outWriter = new RawDnaPartsWriter(dnaFile, outQueue, outPool);

		// launch stuff
		//
		mt::thread readerThread(mt::ref(*inReader));

		std::vector<IOperator*> operators;
		operators.resize(threadsNum_);

#ifdef USE_BOOST_THREAD
		boost::thread_group opThreadGroup;

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			operators[i] = new DnaPartsDecompressor(minParams,
													inQueue, inPool,
													outQueue, outPool);
			opThreadGroup.create_thread(mt::ref(*operators[i]));
		}

		(*outWriter)();

		readerThread.join();
		opThreadGroup.join_all();

#else
		std::vector<th::thread> opThreadGroup;

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			operators[i] = new BinPartsCompressor(conf.minimizer, params_,
												  inQueue, inPool,
												  outQueue, outPool,
												  metaMapper, dnaMapper);
			opThreadGroup.push_back(mt::thread(mt::ref(*operators[i])));
		}

		(*outWriter)();

		readerThread.join();

		for (th::thread& t : opThreadGroup)
		{
			t.join();
		}

#endif

		for (uint32 i = 0; i < threadsNum_; ++i)
		{
			delete operators[i];
		}

		TFREE(outWriter);
		TFREE(inReader);

		TFREE(outQueue);
		TFREE(outPool);
		TFREE(inQueue);
		TFREE(inPool);
	}
	else
	{

		DnaDecompressor compressor(minParams);
		CompressedDnaBlock compBlock;
		DnaParser parser;

		DataChunk dnaChunk;

		while (dnarch->ReadNextBin(&compBlock))
		{
			compressor.DecompressDna(compBlock, compBlock.workBuffers.dnaBin,
									 compBlock.workBuffers.dnaWorkBin, compBlock.workBuffers.dnaBuffer);
			parser.ParseTo(compBlock.workBuffers.dnaBin, dnaChunk);

			dnaFile->Write(dnaChunk.data.Pointer(), dnaChunk.size);
		}
	}

	dnarch->FinishDecompress();
	dnaFile->Close();

	delete dnaFile;
	delete dnarch;
}
