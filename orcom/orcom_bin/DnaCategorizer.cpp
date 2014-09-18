#include "DnaCategorizer.h"

#include <string.h>
#include <algorithm>

#include "DnaBlockData.h"

DnaCategorizer::DnaCategorizer(const MinimizerParameters& params_, const CategorizerParameters& catParams_)
	:	params(params_)
	,	catParams(catParams_)
	,	maxShortMinimValue(1 << (2 * params.signatureSuffixLen))
	,	maxLongMinimValue(1 << (2 * params.signatureLen))
	,	nBinValue(maxLongMinimValue)
{
	ASSERT(params.signatureSuffixLen <= params.signatureLen);

	std::fill(symbolIdxTable, symbolIdxTable + 128, -1);
	for (uint32 i = 0; i < 5; ++i)
		symbolIdxTable[(int32)params.dnaSymbolOrder[i]] = i;

	freqTable.resize(maxShortMinimValue, 0);
}

// this method needs refactoring <-- too much responsibilities
//
void DnaCategorizer::Categorize(std::vector<DnaRecord>& records_, uint64 recordsCount_, DnaBinBlock& bin_)
{
	ASSERT(recordsCount_ > 0);
	ASSERT(recordsCount_ <= records_.size());

	// crear bins
	//
	for (uint32 i = 0; i < bin_.stdBins.Size(); ++i)
		bin_.stdBins[i].Clear();
	bin_.nBin.Clear();

	std::fill(freqTable.begin(), freqTable.end(), 0);


	// process records
	//
	DistributeToBins(records_, recordsCount_, bin_.stdBins, bin_.nBin);

	// sort the bins
	//
	FindMinimizerPositions(bin_.stdBins);

	DnaRecordComparator comparator(params.signatureLen - params.signatureSuffixLen);
	for (uint32 i = 0; i < bin_.stdBins.Size(); ++i)
	{
		DnaBin& db = bin_.stdBins[i];
		if (db.Size() > 0)
			std::sort(db.Begin(), db.End(), comparator);
	}
}

// todo: split into rev and non-rev
void DnaCategorizer::DistributeToBins(std::vector<DnaRecord>& records_, uint64 recordsCount_, DnaBinCollection& bins_, DnaBin& nBin_)
{
	char revBuffer[1024];	// TODO: make size constant depending on the record max len
	DnaRecord rcRec;
	rcRec.dna = revBuffer;
	rcRec.reverse = true;

	if (params.tryReverseCompliment)
	{
		for (uint32 i = 0; i < recordsCount_; ++i)
		{
			DnaRecord& rec = records_[i];
			rec.reverse = false;

			ASSERT(rec.len > 0);

			rec.ComputeRC(rcRec);

			// find and select minimizers
			//
			const uint32 minimizerFwd = FindMinimizer(rec);
			const uint32 minimizerRev = FindMinimizer(rcRec);
			uint32 minimizer = 0;
			bool reverse = false;

			if (minimizerFwd <= minimizerRev)
			{
				minimizer = minimizerFwd;
			}
			else
			{
				minimizer = minimizerRev;
				reverse = true;
			}

			// store record to bin
			//
			if (minimizer != nBinValue)								// !TODO --- find here minimizer pos
			{
				if (reverse)
				{
					rec.reverse = true;
					std::copy(rcRec.dna, rcRec.dna + rec.len, rec.dna);
				}

				bins_[minimizer].Insert(rec);
			}
			else
			{
				rec.reverse = false;
				rec.minimizerPos = 0;
				nBin_.Insert(rec);
			}
		}
	}
	else
	{
		for (uint32 i = 0; i < recordsCount_; ++i)
		{
			DnaRecord& r = records_[i];
			ASSERT(r.len > 0);
			ASSERT(!r.reverse);
			uint32 minimizer = FindMinimizer(r);

			if (minimizer != nBinValue)										// !TODO --- find here minimizer pos
			{
				bins_[minimizer].Insert(r);
			}
			else
			{
				r.minimizerPos = 0;
				nBin_.Insert(r);
			}
		}
	}

	// re-balance bins
	//
	for (uint32 i = 0; i < bins_.Size(); ++i)
	{
		DnaBin& db = bins_[i];
		if (db.Size() == 0 || db.Size() >= catParams.minBlockBinSize)
			continue;

		for (uint32 j = 0; j < db.Size(); ++j)
		{
			DnaRecord& r = db[j];
			r.minimizerPos = 0;

			// un-reverse the record
			if (r.reverse)
			{
				r.ComputeRC(rcRec);
				r.reverse = false;
				std::copy(rcRec.dna, rcRec.dna + r.len, r.dna);
			}

			std::map<uint32, uint16> mins = FindMinimizers(r);
			std::map<uint32, uint16>::iterator mit = mins.begin();

			for ( ; mit != mins.end(); ++mit)
			{
				const uint32 m = mit->first;
				if (bins_[m].Size() >= catParams.minBlockBinSize)// && bins_[m].Size() < maxBinSize)
				{
					r.minimizerPos = mit->second;
					bins_[m].Insert(r);
					break;
				}
			}

			// only one minimizer or we did not find appropriate bin
			if (mit == mins.end())
			{
				nBin_.Insert(r);
			}
		}
		db.Clear();
	}
}

void DnaCategorizer::FindMinimizerPositions(DnaBinCollection& bins_)
{
	for (uint32 i = 0; i < params.TotalMinimizersCount(); ++i)
	{
		if (bins_[i].Size() == 0)
			continue;

		char minString[64] = {0};
		params.GenerateMinimizer(i, minString);

		for (uint32 j = 0; j < bins_[i].Size(); ++j)
		{
			DnaRecord& r = bins_[i][j];
#if EXP_USE_RC_ADV
			const char* beg = r.dna + (r.reverse ? params.skipZoneLen : 0);
			const char* end = r.dna + r.len - (r.reverse ? params.skipZoneLen : 0);
			const char* mi = std::search(beg, end, minString, minString + params.signatureSuffixLen);

			ASSERT(mi != r.dna + r.len);

			r.minimizerPos = mi - r.dna;
			ASSERT((!r.reverse && r.minimizerPos < r.len - params.skipZoneLen) ||
					(r.minimizerPos >= params.skipZoneLen) );
#else
			const char* mi = std::search(r.dna, r.dna + r.len, minString, minString + params.signatureSuffixLen);

			ASSERT(mi != r.dna + r.len);

			r.minimizerPos = mi - r.dna;
			ASSERT(r.minimizerPos < r.len - params.skipZoneLen);
#endif

		}
	}
}

uint32 DnaCategorizer::FindMinimizer(DnaRecord &rec_)
{
	uint32 minimizer = maxLongMinimValue;
	ASSERT(rec_.len >= params.signatureLen - params.skipZoneLen + 1);

#if EXP_USE_RC_ADV
	const int32 ibeg = rec_.reverse ? params.skipZoneLen : 0;
	const int32 iend = rec_.len - params.signatureLen + 1 - (rec_.reverse ? 0 : params.skipZoneLen);
#else
	const int32 ibeg = 0;
	const int32 iend = rec_.len - params.signatureLen + 1 - params.skipZoneLen;
#endif

	for (int32 i = ibeg; i < iend; ++i)
	{
		uint32 m = ComputeMinimizer(rec_.dna + i, params.signatureLen);

		if (m < minimizer && IsMinimizerValid(m, params.signatureLen))
			minimizer = m;
	}

	// invalid minimizer
	if (minimizer >= maxLongMinimValue)
		return nBinValue;

	// apply minimizer mask cut
	return minimizer & (maxShortMinimValue - 1);
}

std::map<uint32, uint16> DnaCategorizer::FindMinimizers(DnaRecord &rec_)
{
	ASSERT(rec_.len >= params.signatureLen - params.skipZoneLen + 1);

	// find all
	std::map<uint32, uint16> signatures;
	for (int32 i = 0; i < rec_.len - params.signatureLen + 1 - params.skipZoneLen; ++i)
	{
		uint32 m = ComputeMinimizer(rec_.dna + i, params.signatureLen);

		if (IsMinimizerValid(m, params.signatureLen) && m < maxLongMinimValue)
		{
			m &= (maxShortMinimValue - 1);
			if (signatures.count(m) == 0)
				signatures[m] = i + (params.signatureLen - params.signatureSuffixLen);
		}
	}

	return signatures;
}

uint32 DnaCategorizer::ComputeMinimizer(const char* dna_, uint32 mLen_)
{
	uint32 r = 0;

	for (uint32 i = 0; i < mLen_; ++i)
	{
		if (dna_[i] == 'N')
			return nBinValue;

		ASSERT(dna_[i] >= 'A' && dna_[i] <= 'T');
		r <<= 2;
		r |= symbolIdxTable[(uint32)dna_[i]];
	}

	return r;
}

bool DnaCategorizer::IsMinimizerValid(uint32 minim_, uint32 mLen_)
{
	if (minim_ == 0)
		minim_ = 0;

	const uint32 excludeLen = 3;
	const uint32 excludeMask = 0x3F;
	const uint32 symbolExcludeTable[] = {0x00, 0x15, 0x2A, 0x3F};

	minim_ &= (1 << (2*mLen_)) - 1;
	bool hasInvalidSeq = false;

	for (uint32 i = 0; !hasInvalidSeq && i <= (mLen_ - excludeLen); ++i)
	{
		uint32 x = minim_ & excludeMask;

		for (uint32 j = 0; j < excludeLen; ++j)
			hasInvalidSeq |= (x == symbolExcludeTable[j]);

		minim_ >>= 2;
	}

	return !hasInvalidSeq;
}
