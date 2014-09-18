#ifndef DNABLOCKDATA_H
#define DNABLOCKDATA_H

#include "Globals.h"

#include "Collections.h"

struct DnaBinBlock
{
	DnaBinCollection stdBins;
	DnaBin nBin;

	DnaBinBlock(uint32 stdBinSize_ = 0)
		:	stdBins(stdBinSize_)
	{}

	void Reset()
	{
		stdBins.Reset();
		nBin.Reset();
	}
};


#endif // DNABLOCKDATA_H
