#ifndef DNAPACKER_H
#define DNAPACKER_H

#include "Globals.h"
#include "Params.h"

#include "BinBlockData.h"

class DnaPacker
{
public:
	DnaPacker(const MinimizerParameters& params_);

	void PackToBins(const DnaBinBlock& dnaBins_, BinaryBinBlock& binBins_);
	void UnpackFromBins(const BinaryBinBlock& binBins_, DnaBinBlock& dnaBins_, DataChunk& dnaChunk_);
	void UnpackFromBin(const BinaryBinBlock& binBin_, DnaBin& dnaBin_, uint32 minimizerId_, DataChunk& dnaChunk_, bool append_ = false);

private:
	struct BinPackSettings
	{
		bool hasConstLen;
		uint32 minLen;
		uint32 maxLen;
		uint32 suffixLen;
		uint32 bitsPerLen;

		BinPackSettings()
			:	hasConstLen(false)
			,	minLen((uint32)-1)
			,	maxLen(0)
			,	suffixLen(0)
			,	bitsPerLen(0)
		{}
	};

	static const uint32 LenBits = 8;

	const MinimizerParameters params;

	char dnaToIdx[128];
	char idxToDna[8];


	void PackToBin(const DnaBin& dnaBin_, BitMemoryWriter& metaWriter_, BitMemoryWriter& dnaWriter_,
				   BinaryBinDescriptor& binDesc_, bool cutMinimizer_ = false);

	void UnpackFromBin(const BinaryBinDescriptor& desc_, DnaBin& dnaBin_, DataChunk& dnaChunk_,
					   BitMemoryReader& metaReader_, BitMemoryReader& dnaReader_, uint32 minimizerId_);

	void StoreNextRecord(const DnaRecord& rec_, BitMemoryWriter& binWriter_, BitMemoryWriter& dnaWriter_, const BinPackSettings& settings_);
	bool ReadNextRecord(BitMemoryReader& binReader_, BitMemoryReader& dnaReader_, DnaRecord& rec_, const BinPackSettings& settings_);

};


#endif // DNAPACKER_H
