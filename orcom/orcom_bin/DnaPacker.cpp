/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "Globals.h"
#include "DnaPacker.h"
#include "BitMemory.h"
#include "DnaBlockData.h"
#include "BinBlockData.h"
#include "Utils.h"


DnaPacker::DnaPacker(const MinimizerParameters& params_)
	:	params(params_)
{
	std::fill(dnaToIdx, dnaToIdx + 128, -1);
	for (uint32 i = 0; i < 5; ++i)
	{
		int32 c = params_.dnaSymbolOrder[i];
		ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
		dnaToIdx[c] = i;
	}

	std::fill(idxToDna, idxToDna + 8, -1);
	for (uint32 i = 0; i < 5; ++i)
	{
		idxToDna[i] = params_.dnaSymbolOrder[i];
	}
}


void DnaPacker::PackToBins(const DnaBinBlock& dnaBins_, BinaryBinBlock& binBins_)
{
	binBins_.Clear();

	BitMemoryWriter metaWriter(binBins_.metaData);
	BitMemoryWriter dnaWriter(binBins_.dnaData);

	// store standard bins
	//
	ASSERT(!dnaBins_.stdBins.Exists(params.TotalMinimizersCount()));
	for (DnaBinCollection::ConstIterator i = dnaBins_.stdBins.CBegin(); i != dnaBins_.stdBins.CEnd(); ++i)
	{
		const uint32 binId = i->first;
		const DnaBin& db = i->second;

		if (db.Size() == 0)
			continue;

		BinaryBinDescriptor& desc = binBins_.descriptors[binId];
		desc.Clear();

		PackToBin(db, metaWriter, dnaWriter, desc, false);
		binBins_.rawDnaSize += desc.rawDnaSize;
	}

	// pack last N bin
	//
	{
		uint32 nBinId = params.TotalMinimizersCount();

		BinaryBinDescriptor& nDesc = binBins_.descriptors[nBinId];
		nDesc.Clear();

		if (dnaBins_.nBin.Size() > 0)
			PackToBin(dnaBins_.nBin, metaWriter, dnaWriter, nDesc, true);

		binBins_.metaSize = metaWriter.Position();
		binBins_.dnaSize = dnaWriter.Position();
		binBins_.rawDnaSize += nDesc.rawDnaSize;
	}
}


void DnaPacker::PackToBin(const DnaBin &dnaBin_, BitMemoryWriter& metaWriter_, BitMemoryWriter& dnaWriter_,
						  BinaryBinDescriptor& binDesc_, bool cutMinimizer_)
{
	const uint32 recordsCount = dnaBin_.Size();
	const uint64 initialMetaPos = metaWriter_.Position();
	const uint64 initialDnaPos = dnaWriter_.Position();

	binDesc_.recordsCount = recordsCount;

	if (recordsCount == 0)
		return;

	ASSERT(recordsCount < (1 << 28));
	ASSERT(dnaBin_.GetStats().maxLen > 0);
	ASSERT(dnaBin_.GetStats().maxLen >= dnaBin_.GetStats().minLen);

	BinPackSettings settings;
	settings.minLen = dnaBin_.GetStats().minLen;
	settings.maxLen = dnaBin_.GetStats().maxLen;
	settings.hasConstLen = (settings.minLen == settings.maxLen);

	if (!cutMinimizer_)
		settings.suffixLen = params.signatureSuffixLen;
	else
		settings.suffixLen = 0;

	metaWriter_.PutBits(dnaBin_.GetStats().minLen, LenBits);
	metaWriter_.PutBits(dnaBin_.GetStats().maxLen, LenBits);

	if (!settings.hasConstLen)
		settings.bitsPerLen = bit_length(dnaBin_.GetStats().maxLen - dnaBin_.GetStats().minLen);

	for (uint32 i = 0; i < recordsCount; ++i)
	{
		const DnaRecord& drec = dnaBin_[i];
		StoreNextRecord(drec, metaWriter_, dnaWriter_, settings);
		binDesc_.rawDnaSize += drec.len;
	}

	metaWriter_.FlushPartialWordBuffer();
	dnaWriter_.FlushPartialWordBuffer();

	binDesc_.metaSize = metaWriter_.Position() - initialMetaPos;
	binDesc_.dnaSize = dnaWriter_.Position() - initialDnaPos;
}


void DnaPacker::StoreNextRecord(const DnaRecord &rec_, BitMemoryWriter& metaWriter_, BitMemoryWriter& dnaWriter_,
								const BinPackSettings& settings_)
{
	ASSERT(rec_.len > 0);

	// store meta info
	//
	const bool isDnaPlain = std::find(rec_.dna, rec_.dna + rec_.len, 'N') == rec_.dna + rec_.len;
	const bool hasMinimizer = settings_.suffixLen != 0;

	metaWriter_.PutBit(isDnaPlain);

	if (hasMinimizer)
	{
		metaWriter_.PutBit(rec_.reverse);
		metaWriter_.PutBits(rec_.minimizerPos, LenBits);
	}
	else
	{
		ASSERT(rec_.minimizerPos == 0);
	}

	if (!settings_.hasConstLen)
		metaWriter_.PutBits(rec_.len - settings_.minLen, settings_.bitsPerLen);


	// when saving record, skip writing bytes identifying the signature
	//
	if (isDnaPlain)
	{
		for (uint32 i = 0 ; i < rec_.minimizerPos; ++i)
			dnaWriter_.Put2Bits(dnaToIdx[(uint32)rec_.dna[i]]);

		for (uint32 i = rec_.minimizerPos + settings_.suffixLen; i < rec_.len; ++i)
			dnaWriter_.Put2Bits(dnaToIdx[(uint32)rec_.dna[i]]);
	}
	else
	{
		for (uint32 i = 0; i < rec_.minimizerPos; ++i)
			dnaWriter_.PutBits(dnaToIdx[(uint32)rec_.dna[i]], 3);

		for (uint32 i = rec_.minimizerPos + settings_.suffixLen; i < rec_.len; ++i)
			dnaWriter_.PutBits(dnaToIdx[(uint32)rec_.dna[i]], 3);
	}
}


void DnaPacker::UnpackFromBins(const BinaryBinBlock &binBins_, DnaBinBlock &dnaBins_, DataChunk& dnaChunk_)
{
	const uint32 minimizersCount = params.TotalMinimizersCount();

	dnaBins_.Clear();

	// calculate total size of streams
	//
	uint64 rawDnaSize = 0;
	for (BinaryBinBlock::DescriptorMap::const_iterator i = binBins_.descriptors.begin();
		 i != binBins_.descriptors.end(); ++i)
	{
		const BinaryBinDescriptor& desc = i->second;
		rawDnaSize += desc.rawDnaSize;
	}

	if (dnaChunk_.data.Size() < rawDnaSize)
		dnaChunk_.data.Extend(rawDnaSize);
	dnaChunk_.size = 0;


	// initialize stream readers
	//
	BitMemoryReader metaReader(binBins_.metaData, binBins_.metaSize);
	BitMemoryReader dnaReader(binBins_.dnaData, binBins_.dnaSize);

	const BinaryBinBlock::DescriptorMap::const_iterator nIter = binBins_.descriptors.find(minimizersCount);
	for (BinaryBinBlock::DescriptorMap::const_iterator i = binBins_.descriptors.begin();
		 i != nIter; ++i)
	{
		const uint32 minId = i->first;
		const BinaryBinDescriptor& binDesc = i->second;

		DnaBin& db = dnaBins_.stdBins[minId];
		db.Clear();

		if (binDesc.recordsCount > 0)
		{
			UnpackFromBin(binDesc, db, dnaChunk_, metaReader, dnaReader, minId);
		}
	}

	dnaBins_.nBin.Clear();
	if (nIter != binBins_.descriptors.end())
	{
		const BinaryBinDescriptor& nBinDesc = nIter->second;

		if (nBinDesc.recordsCount > 0)
			UnpackFromBin(nBinDesc, dnaBins_.nBin, dnaChunk_, metaReader, dnaReader, minimizersCount);
	}

}


void DnaPacker::UnpackFromBin(const BinaryBinDescriptor& desc_, DnaBin &dnaBin_, DataChunk& dnaChunk_,
							  BitMemoryReader& metaReader_, BitMemoryReader& dnaReader_, uint32 minimizerId_)
{
	const uint64 recordsCount = desc_.recordsCount;
	const uint64 initialMetaPos = metaReader_.Position();
	const uint64 initialDnaPos = dnaReader_.Position();

	ASSERT(recordsCount < (1 << 28));
	dnaBin_.Resize(recordsCount);

	// initialize settings
	//
	BinPackSettings settings;
	settings.minLen = metaReader_.GetBits(LenBits);
	settings.maxLen = metaReader_.GetBits(LenBits);
	settings.hasConstLen = (settings.minLen == settings.maxLen);

	ASSERT(settings.maxLen > 0);
	ASSERT(settings.maxLen >= settings.minLen);

	if (minimizerId_ != params.TotalMinimizersCount())
		settings.suffixLen = params.signatureSuffixLen;
	else
		settings.suffixLen = 0;

	if (!settings.hasConstLen)
	{
		settings.bitsPerLen = bit_length(settings.maxLen - settings.minLen);
		ASSERT(settings.bitsPerLen > 0);
	}

	char* dnaBufferPtr = (char*)dnaChunk_.data.Pointer();
	uint64 dnaBufferPos = 0;


	// unpack records
	//
	if (settings.suffixLen > 0)
	{
		char minString[64] = {0};
		params.GenerateMinimizer(minimizerId_, minString);

		for (uint32 r = 0; r < recordsCount; ++r)
		{
			DnaRecord& rec = dnaBin_[r];
			rec.dna = dnaBufferPtr + dnaChunk_.size + dnaBufferPos;

			bool b = ReadNextRecord(metaReader_, dnaReader_, rec, settings);
			ASSERT(b);
			ASSERT(dnaBufferPos + rec.len <= desc_.rawDnaSize);

			std::copy(minString, minString + params.signatureSuffixLen, rec.dna + rec.minimizerPos);
			dnaBufferPos += rec.len;
		}
	}
	else
	{
		for (uint32 r = 0; r < recordsCount; ++r)
		{
			DnaRecord& rec = dnaBin_[r];
			rec.dna = dnaBufferPtr + dnaChunk_.size + dnaBufferPos;

			bool b = ReadNextRecord(metaReader_, dnaReader_, rec, settings);
			ASSERT(b);
			ASSERT(dnaBufferPos + rec.len <= desc_.rawDnaSize);

			dnaBufferPos += rec.len;
		}
	}

	dnaChunk_.size += dnaBufferPos;

	dnaBin_.SetStats(settings.minLen, settings.maxLen);

	metaReader_.FlushInputWordBuffer();
	dnaReader_.FlushInputWordBuffer();

	ASSERT(metaReader_.Position() - initialMetaPos == desc_.metaSize);
	ASSERT(dnaReader_.Position() - initialDnaPos == desc_.dnaSize);
}


void DnaPacker::UnpackFromBin(const BinaryBinBlock& binBin_, DnaBin& dnaBin_, uint32 minimizerId_, DataChunk& dnaChunk_, bool append_)
{
	// calculate global bin properties
	//
	uint64 recordsCount = 0;

	for (BinaryBinBlock::DescriptorMap::const_iterator i = binBin_.descriptors.begin();
		 i != binBin_.descriptors.end(); ++i)
	{
		const BinaryBinDescriptor& desc = i->second;
		recordsCount += desc.recordsCount;
	}

	ASSERT(recordsCount < (1 << 28));

	// initialize
	//
	uint64 recId = 0;
	if (append_)
	{
		recId = dnaBin_.Size();
		recordsCount += recId;

		ASSERT(dnaChunk_.data.Size() >= dnaChunk_.size + binBin_.rawDnaSize);
	}
	else
	{
		dnaBin_.Clear();
		dnaChunk_.size = 0;

		if (dnaChunk_.data.Size() < binBin_.rawDnaSize)
			dnaChunk_.data.Extend(binBin_.rawDnaSize);
	}
	dnaBin_.Resize(recordsCount);

	if (recordsCount == 0)
		return;


	// configure settings
	//
	BinPackSettings settings;
	char minString[64] = {0};
	if (minimizerId_ != params.TotalMinimizersCount())	// N bin?
	{
		settings.suffixLen = params.signatureSuffixLen;
		params.GenerateMinimizer(minimizerId_, minString);
	}
	else
	{
		settings.suffixLen = 0;
	}

	BitMemoryReader metaReader(binBin_.metaData, binBin_.metaSize);
	BitMemoryReader dnaReader(binBin_.dnaData, binBin_.dnaSize);

	char* dnaBufferPtr = (char*)dnaChunk_.data.Pointer();

	for (BinaryBinBlock::DescriptorMap::const_iterator i = binBin_.descriptors.begin();
		 i != binBin_.descriptors.end(); ++i)
	{
		const BinaryBinDescriptor& desc = i->second;

		const uint64 initialMetaPos = metaReader.Position();
		const uint64 initialDnaPos = dnaReader.Position();

		// read bin header
		//
		settings.minLen = metaReader.GetBits(LenBits);
		settings.maxLen = metaReader.GetBits(LenBits);
		ASSERT(settings.minLen > 0);
		ASSERT(settings.maxLen >= settings.minLen);

		settings.hasConstLen = (settings.minLen == settings.maxLen);

		if (!settings.hasConstLen)
		{
			settings.bitsPerLen = bit_length(settings.maxLen - settings.minLen);
			ASSERT(settings.bitsPerLen > 0);
		}

		// read bin records
		//
		if (settings.suffixLen > 0)
		{
			for (uint32 j = 0; j < desc.recordsCount; ++j)
			{
				DnaRecord& rec = dnaBin_[recId++];
				rec.dna = dnaBufferPtr + dnaChunk_.size;

				bool b = ReadNextRecord(metaReader, dnaReader, rec, settings);
				ASSERT(b);
				ASSERT(dnaChunk_.size + rec.len <= dnaChunk_.data.Size());


				std::copy(minString, minString + params.signatureSuffixLen, rec.dna + rec.minimizerPos);
				dnaChunk_.size += rec.len;
			}
		}
		else
		{
			for (uint32 j = 0; j < desc.recordsCount; ++j)
			{
				DnaRecord& rec = dnaBin_[recId++];
				rec.dna = dnaBufferPtr + dnaChunk_.size;

				bool b = ReadNextRecord(metaReader, dnaReader, rec, settings);
				ASSERT(b);
				ASSERT(dnaChunk_.size + rec.len <= dnaChunk_.data.Size());

				dnaChunk_.size += rec.len;
			}
		}

		metaReader.FlushInputWordBuffer();
		dnaReader.FlushInputWordBuffer();

		dnaBin_.SetStats(MIN(settings.minLen, dnaBin_.GetStats().minLen),
						 MAX(settings.maxLen, dnaBin_.GetStats().maxLen));

		ASSERT(metaReader.Position() - initialMetaPos == desc.metaSize);
		ASSERT(dnaReader.Position() - initialDnaPos == desc.dnaSize);
	}
}


bool DnaPacker::ReadNextRecord(BitMemoryReader &metaReader_, BitMemoryReader &dnaReader_, DnaRecord &rec_,
							   const BinPackSettings& settings_)
{
	if (dnaReader_.Position() >= dnaReader_.Size())
		return false;

	// read general record info
	//
	const bool isDnaPlain = metaReader_.GetBit() != 0;
	const bool hasMinimizer = settings_.suffixLen != 0;

	if (hasMinimizer)
	{
		rec_.reverse = metaReader_.GetBit() != 0;
		rec_.minimizerPos = metaReader_.GetBits(LenBits);
		ASSERT(rec_.minimizerPos < DnaRecord::MaxDnaLen);
	}
	else
	{
		rec_.reverse = false;
		rec_.minimizerPos = 0;
	}

	if (settings_.hasConstLen)
	{
		rec_.len = settings_.minLen;
	}
	else
	{
		rec_.len = metaReader_.GetBits(settings_.bitsPerLen) + settings_.minLen;
	}
	ASSERT(rec_.len > 0 && rec_.len < DnaRecord::MaxDnaLen);

	if (isDnaPlain)
	{
		for (uint32 i = 0; i < rec_.minimizerPos; ++i)
		{
			rec_.dna[i] = idxToDna[dnaReader_.Get2Bits()];
			ASSERT(rec_.dna[i] != -1);
		}

		for (uint32 i = rec_.minimizerPos + settings_.suffixLen; i < rec_.len; ++i)
		{
			rec_.dna[i] = idxToDna[dnaReader_.Get2Bits()];
			ASSERT(rec_.dna[i] != -1);
		}
	}
	else
	{
		for (uint32 i = 0; i < rec_.minimizerPos; ++i)
		{
			rec_.dna[i] = idxToDna[dnaReader_.GetBits(3)];
			ASSERT(rec_.dna[i] != -1);
		}

		for (uint32 i = rec_.minimizerPos + settings_.suffixLen; i < rec_.len; ++i)
		{
			rec_.dna[i] = idxToDna[dnaReader_.GetBits(3)];
			ASSERT(rec_.dna[i] != -1);
		}
	}

	return true;
}
