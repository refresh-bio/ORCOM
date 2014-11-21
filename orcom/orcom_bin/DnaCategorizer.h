/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_DNACATEGORIZER
#define H_DNACATEGORIZER

#include "Globals.h"

#include <vector>
#include <map>

#include "DnaRecord.h"
#include "Collections.h"
#include "Params.h"


class DnaCategorizer
{
public:
	DnaCategorizer(const MinimizerParameters& params_, const CategorizerParameters& catParams_);

	void Categorize(std::vector<DnaRecord>& records_, uint64 recordsCount_, DnaBinBlock& bin_);

private:
	const MinimizerParameters& params;
	const CategorizerParameters catParams;

	const uint32 maxShortMinimValue;
	const uint32 maxLongMinimValue;
	const uint32 nBinValue;

	char symbolIdxTable[128];

	void DistributeToBins(std::vector<DnaRecord>& records_, uint64 recordsCount_, DnaBinCollection& bins_, DnaBin& nBin_);
	void FindMinimizerPositions(DnaBinCollection& bins_);

	uint32 FindMinimizer(DnaRecord& rec_);
	std::map<uint32, uint16> FindMinimizers(DnaRecord &rec_);
	uint32 ComputeMinimizer(const char* dna_, uint32 mLen_);

	bool IsMinimizerValid(uint32 minim_, uint32 mLen_);
};


#endif // H_DNACATEGORIZER
