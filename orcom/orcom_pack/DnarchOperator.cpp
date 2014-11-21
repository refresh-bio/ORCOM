/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "../orcom_bin/Globals.h"

#include "DnarchOperator.h"
#include "BinFileExtractor.h"
#include "DnarchFile.h"
#include "DnaCompressor.h"
#include "../orcom_bin/DnaPacker.h"
#include "../orcom_bin/DnaParser.h"


void BinPartsExtractor::Run()
{
	int64 partId = 0;

	uint32 minId = 0;
	MinimizerBinPart* part = NULL;
	partsPool->Acquire(part);

	while (partsStream->ExtractNextStdBin(*part, minId))
	{
		if (part->metaSize == 0)
			continue;

		part->minimizer = minId;
		partsQueue->Push(partId++, part);

		part = NULL;
		partsPool->Acquire(part);
	}
	partsPool->Release(part);

	partsQueue->SetCompleted();
}


void BinPartsCompressor::Run()
{
	int64 partId = 0;

	DnaPacker packer(minimizer);
	DnaCompressor compressor(minimizer, params);

	MinimizerBinPart* inPart = NULL;
	while (inPartsQueue->Pop(partId, inPart))
	{
		ASSERT(inPart->metaSize > 0);
		ASSERT(inPart->dnaSize > 0);
		ASSERT(inPart->rawDnaSize > 0);

		const uint32 minimizer = inPart->minimizer;

		CompressedDnaBlock* outPart = NULL;
		outPartsPool->Acquire(outPart);

		outPart->workBuffers.dnaBin.Clear();

		packer.UnpackFromBin(*inPart, outPart->workBuffers.dnaBin, minimizer, outPart->workBuffers.dnaBuffer);

		const uint64 rawDnaSize = inPart->rawDnaSize;
		inPartsPool->Release(inPart);
		inPart = NULL;

		compressor.CompressDna(outPart->workBuffers.dnaBin, minimizer, rawDnaSize, outPart->workBuffers.dnaWorkBin, *outPart);

		outPartsQueue->Push(partId, outPart);
	}

	outPartsQueue->SetCompleted();
}


void DnarchPartsWriter::Run()
{
	int64 partId = 0;
	CompressedDnaBlock* part = NULL;

	while (partsQueue->Pop(partId, part))
	{
		partsStream->WriteNextBin(part);

		partsPool->Release(part);
		part = NULL;
	}
}


void DnarchPartsReader::Run()
{
	int64 partId = 0;
	PartType* part = NULL;
	partsPool->Acquire(part);

	while (partsStream->ReadNextBin(part))
	{
		ASSERT(part->dataBuffer.size > 0);		// hack

		partsQueue->Push(partId, part);
		part = NULL;

		partsPool->Acquire(part);
		partId++;
	}
	partsPool->Release(part);
	partsQueue->SetCompleted();
}


void DnaPartsDecompressor::Run()
{
	DnaDecompressor compressor(minimizer);
	DnaParser parser;

	int64 partId = 0;
	InPartType* inPart = NULL;

	while (inPartsQueue->Pop(partId, inPart))
	{
		OutPartType* outPart = NULL;
		outPartsPool->Acquire(outPart);

		compressor.DecompressDna(*inPart, inPart->workBuffers.dnaBin, inPart->workBuffers.dnaWorkBin, inPart->workBuffers.dnaBuffer);

		parser.ParseTo(inPart->workBuffers.dnaBin, *outPart);		// TODO: refactor, skip this step
		outPartsQueue->Push(partId, outPart);

		inPartsPool->Release(inPart);
		inPart = NULL;
	}
	outPartsQueue->SetCompleted();
}


void RawDnaPartsWriter::Run()
{
	int64 partId = 0;
	PartType* part = NULL;

	while (partsQueue->Pop(partId, part))
	{
		ASSERT(part->size > 0);

		partsStream->Write(part->data.Pointer(), part->size);

		partsPool->Release(part);
	}
}
