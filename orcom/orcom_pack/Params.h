#ifndef PACKPARAMS_H
#define PACKPARAMS_H

#include "../orcom_bin/Globals.h"

struct CompressorParams
{
	static const int32 DefaultMaxCostValue = (uint16)-1;
	static const int32 DefaultEncodeThresholdValue = 0;
	static const int32 DefaultMismatchCost = 2;
	static const int32 DefaultInsertCost = 1;
	static const uint32 DefaultMinimumBinSize = 64;

	int32 maxCostValue;
	int32 encodeThresholdValue;
	int32 mismatchCost;
	int32 insertCost;
	uint32 minBinSize;

	CompressorParams()
		:	maxCostValue(DefaultMaxCostValue)
		,	encodeThresholdValue(DefaultEncodeThresholdValue)
		,	mismatchCost(DefaultMismatchCost)
		,	insertCost(DefaultInsertCost)
		,	minBinSize(DefaultMinimumBinSize)
	{}

	static CompressorParams Default()
	{
		CompressorParams pars;
		return pars;
	}
};


#endif // PACKPARAMS_H
