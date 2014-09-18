#include "Globals.h"

#include <vector>
#include <map>

#include "BinOperator.h"
#include "FastqStream.h"
#include "BinFile.h"
#include "DnaParser.h"
#include "DnaCategorizer.h"
#include "DnaPacker.h"
#include "DnaBlockData.h"


void FastqChunkReader::Run()
{
	uint64 partId = 0;

#if 0										// TODO: test this routine
	while (!partsStream->Eof())
	{
		DataChunk* part = NULL;
		partsPool->Acquire(part);

		partsStream->ReadNextChunk(part);

		if (part->size > 0)
			partsQueue->Push(partId++, part);
		else
		{
			partsPool->Release(part);
			break;
		}
	}
#else

	DataChunk* part = NULL;
	partsPool->Acquire(part);

	while (partsStream->ReadNextChunk(part))
	{
		partsQueue->Push(partId++, part);

		partsPool->Acquire(part);
	}

	partsPool->Release(part);

#endif
	partsQueue->SetCompleted();
}

void BinChunkWriter::Run()
{
	int64 partId = 0;
	BinaryPart* part = NULL;

	while (partsQueue->Pop(partId, part))
	{
		// here PartId is not important
		partsStream->WriteNextBlock(part);

		partsPool->Release(part);
		part = NULL;
	}
}

void BinEncoder::Run()
{
	DnaPacker packer(params);
	DnaCategorizer categorizer(params, catParams);

	int64 partId = 0;
	DataChunk* fqPart = NULL;
	BinaryPart* binPart = NULL;

	DnaBinBlock dnaBins(params.TotalMinimizersCount());
	std::vector<DnaRecord> records;
	records.resize(1 << 10);

	DnaParser parser;

	DataChunk dnaBuffer;

	while (fqPartsQueue->Pop(partId, fqPart))
	{
		uint64 rc = 0;

		parser.ParseFrom(*fqPart, dnaBuffer, records, rc);

		fqPartsPool->Release(fqPart);

		categorizer.Categorize(records, rc, dnaBins);

		binPartsPool->Acquire(binPart);

		packer.PackToBins(dnaBins, *binPart);

		binPartsQueue->Push(partId, binPart);
	}

	binPartsQueue->SetCompleted();
}
