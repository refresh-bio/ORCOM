/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_BINCOMPRESSOR
#define H_BINCOMPRESSOR

#include "../orcom_bin/Globals.h"

#include <string>
#include <deque>

#include "Params.h"
#include "CompressedBlockData.h"

#include "../orcom_bin/BitMemory.h"
#include "../orcom_bin/Params.h"
#include "../rle/RleEncoder.h"
#include "../rc/ContextEncoder.h"
#include "../ppmd/PPMd.h"


class DnaStoreBase
{
public:
	DnaStoreBase(const MinimizerParameters& minParams_);
	~DnaStoreBase();

protected:
	enum ReadFlags
	{
		ReadIdentical = 0,
		ReadDifficult,
		ReadFullEncode,
		ReadShiftOnly,
		ReadLastPosDifference
	};

	struct LzMatch
	{
		char* seq;
		int32 seqLen;
		int32 minPos;

		LzMatch()
			:	seq(NULL)
			,	seqLen(0)
			,	minPos(0)
		{}
	};

	struct DnaShortBlockHeader
	{
		static const uint32 Size = sizeof(uint32) + 2*sizeof(uint64) + 2*sizeof(uint8);

		uint64 recordsCount;
		uint64 rawDnaStreamSize;

		uint32 minimizerId;

		uint8 recMinLen;
		uint8 recMaxLen;


		DnaShortBlockHeader()
		{
			Clear();
		}

		void Clear()
		{
			minimizerId = 0;
			recordsCount = 0;

			recMinLen = (uint8)-1;
			recMaxLen = 0;

			rawDnaStreamSize = 0;
		}
	};

	struct DnaBlockHeader : public DnaShortBlockHeader
	{
		static const uint32 BuffersCount = DnaCompressedBin::BuffersNum;
		static const uint32 Size = DnaShortBlockHeader::Size + 2*BuffersCount*sizeof(uint64);

		uint64 workBufferSizes[BuffersCount];
		uint64 compBufferSizes[BuffersCount];

		DnaBlockHeader()
		{
			Clear();
		}

		void Clear()
		{
			DnaShortBlockHeader::Clear();
			std::fill(workBufferSizes, workBufferSizes + BuffersCount, 0);
			std::fill(compBufferSizes, compBufferSizes + BuffersCount, 0);
		}
	};

	struct DnaBlockDescription
	{
		DnaBlockHeader header;
		bool isLenConst;

		DnaBlockDescription()
		{
			Clear();
		}

		void Clear()
		{
			header.Clear();
			isLenConst = true;
		}
	};


	// encoders
	//
	typedef TSimpleContextCoder<8, 4> FlagContextCoder;
	typedef TSimpleContextCoder<2, 4> RevContextCoder;
	typedef TAdvancedContextCoder<8, 4> LettersContextCoder;


	// predefined constants
	//
	static const uint32 LzBufferSize = 255;
	static const int32 ShiftOffset = 128;
	static const uint32 InitialSeqLen = DnaRecord::MaxDnaLen;
	static const uint32 MaxSignatureLen = 255;
	static const uint32 MinimizerPositionSymbol = '.';


	// ppmd configuration
	//
	static const uint32 PpmdMemorySizeMb = 16;
	static const uint32 PpmdOrder = 4;


	//  members
	//
	const MinimizerParameters minParams;

	char dnaToIdx[128];
	char idxToDna[5];
	char dummySequence[DnaRecord::MaxDnaLen];

	DnaBlockDescription blockDesc;
	std::deque<LzMatch*> prevBuffer;
	char currentMinimizerBuf[MaxSignatureLen];


	// methods
	//
	void PrepareLzBuffer(uint32 size_);
};


class DnaCompressorBase: public DnaStoreBase
{
public:
	DnaCompressorBase(const MinimizerParameters& minParams_, const CompressorParams& compParams_)
		:	DnaStoreBase(minParams_)
		,	compParams(compParams_)
	{}

protected:
	struct MatchResult
	{
		int32 cost;
		int32 prevId;
		int32 shift;
		bool noMismatches;

		MatchResult()
			:	cost(0)
			,	prevId(0)
			,	shift(0)
			,	noMismatches(false)
		{}
	};

	static const int32 MaxShiftValue = ShiftOffset-1;

	const CompressorParams compParams;

	MatchResult FindBestLzMatch(const DnaRecord& rec_, int32 recMinPos_);
	int32 CalculateMismatchesCost(const char* seq1_, uint32 len1_, const char* seq2_, uint32 len2_, int32 bestValue_);
};


class DnaCompressor : public DnaCompressorBase
{
public:
	DnaCompressor(const MinimizerParameters& minParams_, const CompressorParams& compParams_);
	~DnaCompressor();

	void CompressDna(DnaBin& dnaBin_, uint32 minimizerId_, uint64 rawDnaStreamSize_, DnaCompressedBin& dnaWorkBin, CompressedDnaBlock& compBin_);

private:
	typedef TEncoder<FlagContextCoder> FlagEncoder;
	typedef TEncoder<RevContextCoder> RevEncoder;
	typedef TEncoder<LettersContextCoder> LettersEncoder;

	BinaryRleEncoder* rleEncoder;
	FlagEncoder* flagCoder;
	RevEncoder* revCoder;
	LettersEncoder* lettersCoder;
	PpmdEncoder* ppmdEncoder;

	std::vector<BitMemoryWriter*> writers;

	void PrepareWriters(DnaCompressedBin& dnaWorkBin_);
	void CleanupWriters();

	void CompressDnaFull(DnaBin& dnaBin_, DnaCompressedBin& dnaWorkBin_, CompressedDnaBlock& compBin_);
	void CompressDnaRaw(DnaBin& dnaBin_, DnaCompressedBin& dnaWorkBin_, CompressedDnaBlock& compBin_);

	void CompressRecordNormal(const DnaRecord& rec_);
};


class DnaDecompressor : public DnaStoreBase
{
public:
	DnaDecompressor(const MinimizerParameters& minParams_);
	~DnaDecompressor();

	void DecompressDna(CompressedDnaBlock& compBin_, DnaBin& dnaBin_, DnaCompressedBin& dnaWorkBin_, DataChunk& dnaBuffer_);

private:
	typedef TDecoder<FlagContextCoder> FlagDecoder;
	typedef TDecoder<RevContextCoder> RevDecoder;
	typedef TDecoder<LettersContextCoder> LettersDecoder;

	BinaryRleDecoder* rleDecoder;
	FlagDecoder* flagCoder;
	RevDecoder* revCoder;
	LettersDecoder* lettersCoder;
	PpmdDecoder* ppmdDecoder;

	std::vector<BitMemoryReader*> readers;

	void PrepareReaders(DnaCompressedBin& dnaWorkBin_);
	void CleanupReaders();

	void DecompressDnaFull(CompressedDnaBlock& compBin_, DnaCompressedBin& dnaWorkBin_, DnaBin& dnaBin_, DataChunk& dnaBuffer_);
	void DecompressDnaRaw(CompressedDnaBlock& compBin_, DnaCompressedBin& dnaWorkBin_, DnaBin& dnaBin_, DataChunk& dnaBuffer_);

	void DecompressRecordNormal(DnaRecord& rec_);
};


#endif // H_BINCOMPRESSOR
