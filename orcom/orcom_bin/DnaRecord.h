#ifndef DNARECORD_H
#define DNARECORD_H

#include "Globals.h"

#include <string.h>


#pragma pack(push, 4)	// do 4B alignment

struct DnaRecord
{
	static const uint32 MaxDnaLen = 255;

	char* dna;								// 8B
	uint16 len;								// 2B
	uint8 minimizerPos;						// 1B
	bool reverse;							// 1B
	//
	// 4B padding by default !

	DnaRecord()
		:	dna(NULL)
		,	len(0)
		,	minimizerPos(0)
		,	reverse(false)
	{}

	DnaRecord(const DnaRecord& r_)
		:	dna(r_.dna)
		,	len(r_.len)
		,	minimizerPos(r_.minimizerPos)
		,	reverse(r_.reverse)
	{}

	void ComputeRC(DnaRecord& rc_) const
	{
		const char rcCodes[24] = {	-1,'T',-1,'G',-1,-1,-1,'C',		// 64+
									-1,-1,-1,-1,-1,-1,'N',-1,		// 72+,
									-1,-1,-1,-1,'A',-1,-1,-1,		// 80+
								};
		rc_.len = len;
		rc_.reverse = true;
		for (uint32 i = 0; i < len; ++i)
		{
			ASSERT(dna[i] > 64 && dna[i] < 88);
			rc_.dna[len-1-i] = rcCodes[(int32)dna[i] - 64];
			ASSERT(rc_.dna[len-1-i] != -1);
		}
	}
};

#pragma pack(pop)


struct DnaRecordComparator
{
	DnaRecordComparator(uint32 signatureShift_)
		:	signatureShift(signatureShift_)
	{}

	bool operator() (const DnaRecord& r1_, const DnaRecord& r2_)
	{
		const char* r1p = r1_.dna + r1_.minimizerPos - signatureShift;
		const char* r2p = r2_.dna + r2_.minimizerPos - signatureShift;
		uint32 len = MIN(r1_.len - r1_.minimizerPos, r2_.len - r2_.minimizerPos) + signatureShift;

		int32 r = strncmp(r1p, r2p, len);
		if (r == 0)
			return r1_.minimizerPos < r2_.minimizerPos;
		return r < 0;
	}

	const uint32 signatureShift;
};



#endif // DNARECORD_H
