/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_BINPARAMS
#define H_BINPARAMS

#include "Globals.h"

#include <algorithm>


struct MinimizerParameters
{
	static const uint32 DefaultSignatureLen = 8;
	static const uint32 DefaultSignatureSuffixLen = 8;
	static const uint32 DefaultskipZoneLen = 12;
	static const bool DefaultTryRevCompl = true;

	uint8 signatureLen;
	uint8 signatureSuffixLen;
	uint8 skipZoneLen;
	char dnaSymbolOrder[5];
	uint8 tryReverseCompliment;

	MinimizerParameters()
		:	signatureLen(DefaultSignatureLen)
		,	signatureSuffixLen(DefaultSignatureSuffixLen)
		,	skipZoneLen(DefaultskipZoneLen)
		,	tryReverseCompliment(DefaultTryRevCompl)
	{
		dnaSymbolOrder[0] = 'A';
		dnaSymbolOrder[1] = 'C';
		dnaSymbolOrder[2] = 'G';
		dnaSymbolOrder[3] = 'T';
		dnaSymbolOrder[4] = 'N';
	}

	MinimizerParameters(uint32 minLen_, uint32 suffLen_, uint32 cutoffLen_, const char* symOrder_ = "ACGTN", bool tryRc_ = true)
		:	signatureLen(minLen_)
		,	signatureSuffixLen(suffLen_)
		,	skipZoneLen(cutoffLen_)
		,	tryReverseCompliment((uint8)tryRc_)
	{
		std::copy(symOrder_, symOrder_ + 5, dnaSymbolOrder);
	}

	uint64 TotalMinimizersCount() const
	{
		return 1 << (signatureSuffixLen * 2);
	}

	void GenerateMinimizer(uint32 minimizerId_, char* buf_)	const // WARNING: no boundary check !!!
	{
		for (int32 i = signatureSuffixLen - 1; i >= 0; --i)
		{
			buf_[i] = dnaSymbolOrder[minimizerId_ & 0x03];
			minimizerId_ >>= 2;
		}
	}
};


struct CategorizerParameters
{
	static const uint32 DefaultMinimumPartialBinSize = 4;

	uint32 minBlockBinSize;

	CategorizerParameters()
		:	minBlockBinSize(DefaultMinimumPartialBinSize)
	{}
};


struct BinModuleConfig
{
	static const uint64 DefaultFastqBlockSize = 1 << 28;	// 256 MB

	uint64 fastqBlockSize;
	MinimizerParameters minimizer;
	CategorizerParameters catParams;

	BinModuleConfig()
		:	fastqBlockSize(DefaultFastqBlockSize)
	{}
};


#endif // H_BINPARAMS
