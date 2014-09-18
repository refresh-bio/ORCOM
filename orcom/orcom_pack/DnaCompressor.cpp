#include "../orcom_bin/Globals.h"

#include <string.h>

#include "DnaCompressor.h"
#include "CompressedBlockData.h"

#include "../orcom_bin/DnaPacker.h"
#include "../rle/rle.h"
#include "../ppmd/PPMd.h"


DnaStoreBase::DnaStoreBase(const MinimizerParameters &minParams_)
	:	minParams(minParams_)
{
	std::fill(currentMinimizerBuf, currentMinimizerBuf + MaxSignatureLen, 0);

	std::fill(dnaToIdx, dnaToIdx + 128, -1);
	dnaToIdx['A'] = 0;
	dnaToIdx['G'] = 1;
	dnaToIdx['C'] = 2;
	dnaToIdx['T'] = 3;
	dnaToIdx['N'] = 4;

	idxToDna[0] = 'A';
	idxToDna[1] = 'G';
	idxToDna[2] = 'C';
	idxToDna[3] = 'T';
	idxToDna[4] = 'N';

	std::fill(dummySequence, dummySequence + DnaRecord::MaxDnaLen, 'N');
}

DnaStoreBase::~DnaStoreBase()
{
	for (uint32 i = 0; i < prevBuffer.size(); ++i)
		delete prevBuffer[i];
}

void DnaStoreBase::PrepareLzBuffer(uint32 size_)
{
	for (uint32 i = 0; i < prevBuffer.size(); ++i)
		delete prevBuffer[i];
	prevBuffer.clear();

	for (uint32 i = 0; i < size_; ++i)
	{
		LzMatch* lz = new LzMatch();
		lz->seqLen = InitialSeqLen;
		lz->seq = dummySequence;
		prevBuffer.push_back(lz);
	}
}

DnaCompressorBase::MatchResult DnaCompressorBase::FindBestLzMatch(const DnaRecord &rec_, int32 recMinPos_)
{
#define ONLY_POS(x)	((x) > 0 ? (x) : 0)

	MatchResult result;
	result.cost = compParams.maxCostValue;

	for (uint32 i = 0; i < prevBuffer.size(); ++i)
	{
		const LzMatch& lz = *prevBuffer[i];

		// compare
		int32 shift = lz.minPos - recMinPos_;
#if EXP_USE_RC_ADV
		if (ABS(shift) >= lz.seqLen - (rec_.reverse ? 0 : minParams.skipZoneLen) || ABS(shift) > MaxShiftValue)
			continue;
#else
		if (ABS(shift) >= lz.seqLen || ABS(shift) > MaxShiftValue)
			continue;
#endif

		if (shift >= 0)
		{
			int32 insertCost = ONLY_POS(rec_.len - (lz.seqLen - shift)) * compParams.insertCost;
			int32 r = CalculateMismatchesCost(lz.seq + shift,
											  lz.seqLen - shift,
											  rec_.dna,
											  rec_.len,						// TODO:
											  result.cost - insertCost);		// here we can use s1+k+MIN_LEN and s2+MIN_LEN
			bool noMismatches = (r == 0);									// to skip comparison of equal minimizer strings
			r += insertCost;

			if (r < result.cost)
			{
				result.prevId = i;
				result.cost = r;
				result.shift = shift;
				result.noMismatches = noMismatches;
			}
		}
		else
		{
			shift = -shift;

			int32 insertCost = shift + ONLY_POS(rec_.len - shift - lz.seqLen) * compParams.insertCost;
			int32 r = CalculateMismatchesCost(lz.seq,
											  lz.seqLen,
											  rec_.dna + shift,
											  rec_.len - shift,
											  result.cost - insertCost);
			bool noMismatches = (r == 0);
			r += insertCost;
			if (r < result.cost)
			{
				result.prevId = i;
				result.cost = r;
				result.shift = -shift;
				result.noMismatches = noMismatches;
			}
		}
	}

	return result;

#undef ONLY_POS
}

int32 DnaCompressorBase::CalculateMismatchesCost(const char* seq1_, uint32 len1_, const char* seq2_, uint32 len2_, int32 bestValue_)
{
	uint32 minLen = MIN(len1_, len2_);
	int32 v = 0;

	for (uint32 i = 0; i < minLen && v < bestValue_; ++i)
	{
		v += (seq1_[i] != seq2_[i]);
	}
	return v * compParams.mismatchCost;
}


DnaCompressor::DnaCompressor(const MinimizerParameters &minParams_, const CompressorParams &compParams_)
	:	DnaCompressorBase(minParams_, compParams_)
	,	rleEncoder(NULL)
	,	flagCoder(NULL)
	,	revCoder(NULL)
	,	lettersCoder(NULL)
	,	ppmdEncoder(NULL)
{
	// TODO: refactor -- we can initialize all writers and encoders in ctor
	//
	ppmdEncoder = new PpmdEncoder();
	ppmdEncoder->StartCompress(PpmdOrder, PpmdMemorySizeMb);
}

DnaCompressor::~DnaCompressor()
{
	CleanupWriters();	// in case of exception

	ppmdEncoder->FinishCompress();
	delete ppmdEncoder;
}

void DnaCompressor::PrepareWriters(DnaCompressedBin& dnaWorkBin_)
{
	ASSERT(writers.size() == 0);

	for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
	{
		writers.push_back(new BitMemoryWriter(dnaWorkBin_.buffers[i]->data));
		dnaWorkBin_.buffers[i]->size = 0;
	}

	flagCoder = new FlagEncoder(*writers[DnaCompressedBin::FlagBuffer]);
	revCoder = new RevEncoder(*writers[DnaCompressedBin::RevBuffer]);
	lettersCoder = new LettersEncoder(*writers[DnaCompressedBin::LetterXBuffer]);
	rleEncoder = new BinaryRleEncoder(*writers[DnaCompressedBin::MatchBuffer]);
}

void DnaCompressor::CleanupWriters()
{
	TFree(lettersCoder);
	TFree(revCoder);
	TFree(flagCoder);
	TFree(rleEncoder);

	for (uint32 i = 0; i < writers.size(); ++i)
		delete writers[i];

	writers.clear();
}


void DnaCompressor::CompressDna(DnaBin &dnaBin_, uint32 minimizerId_, uint64 rawDnaStreamSize_, DnaCompressedBin& dnaWorkBin_, CompressedDnaBlock &compBin_)
{
	if (dnaBin_.Size() == 0)
		return;

	ASSERT(dnaBin_.stats.maxLen > 0);
	ASSERT(dnaBin_.stats.maxLen >= dnaBin_.stats.minLen);


	// preprocess header
	//
	blockDesc.Clear();
	blockDesc.header.minimizerId = minimizerId_;
	blockDesc.header.recordsCount = dnaBin_.Size();
	blockDesc.header.recMinLen = dnaBin_.stats.minLen;
	blockDesc.header.recMaxLen = dnaBin_.stats.maxLen;
	blockDesc.header.rawDnaStreamSize = rawDnaStreamSize_;
	blockDesc.isLenConst = (dnaBin_.stats.minLen == dnaBin_.stats.maxLen);

	if (minimizerId_ != minParams.TotalMinimizersCount())
		CompressDnaFull(dnaBin_, dnaWorkBin_, compBin_);
	else
		CompressDnaRaw(dnaBin_, dnaWorkBin_, compBin_);

	compBin_.signatureId = minimizerId_;
}

void DnaCompressor::CompressDnaFull(DnaBin &dnaBin_, DnaCompressedBin& dnaWorkBin_, CompressedDnaBlock &compBin_)
{
	// sort records for better lz matching
	//
	DnaRecordComparator comparator(minParams.signatureLen - minParams.signatureSuffixLen);
	std::sort(dnaBin_.Begin(), dnaBin_.End(), comparator);

	PrepareLzBuffer(LzBufferSize);


	// select appropriate scheme
	//
	minParams.GenerateMinimizer(blockDesc.header.minimizerId, currentMinimizerBuf);
	currentMinimizerBuf[minParams.signatureSuffixLen] = 0;


	// prepare writers and start encoding
	//
	PrepareWriters(dnaWorkBin_);

	rleEncoder->Start();
	flagCoder->Start();
	revCoder->Start();
	lettersCoder->Start();

	for (uint64 i = 0; i < dnaBin_.Size(); ++i)
	{
		CompressRecordNormal(dnaBin_[i]);
	}

	rleEncoder->End();
	flagCoder->End();
	revCoder->End();
	lettersCoder->End();


	// flush remaining data
	//
	for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
	{
		writers[i]->FlushPartialWordBuffer();
		dnaWorkBin_.buffers[i]->size = writers[i]->Position();
	}


	// prepare output buffer and gather block info
	//
	{
		uint64 rawOutSize = 0;
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
		{
			rawOutSize += dnaWorkBin_.buffers[i]->size;
			blockDesc.header.workBufferSizes[i] = dnaWorkBin_.buffers[i]->size;
		}

		// usually the output data is compressed to 1/3-1/4 of the input data, but we want to keep the margin
		uint64 preallocSize = rawOutSize;

		if (preallocSize > (1 << 25))
			preallocSize /= 2;
		else if (preallocSize <= (1 << 10))
			preallocSize *= 2;

		if (compBin_.dataBuffer.data.Size() < preallocSize)
			compBin_.dataBuffer.data.Extend(preallocSize);
	}


	// write header and compress
	//
	{
		BitMemoryWriter blockWriter(compBin_.dataBuffer.data);

		// reseve space for header
		blockWriter.FillBytes(0, DnaBlockHeader::Size);

		byte* outMemBegin = compBin_.dataBuffer.data.Pointer();
		uint64 outMemPos = blockWriter.Position();

		// copy range-coded buffers
		//
		{
			uint32 ids[] = {
							DnaCompressedBin::FlagBuffer,
							DnaCompressedBin::RevBuffer,
							DnaCompressedBin::LetterXBuffer
						  };

			uint32 elems = sizeof(ids) / sizeof(uint32);
			for (uint32 i = 0; i < elems; ++i)
			{
				DataChunk& buf = *dnaWorkBin_.buffers[ids[i]];
				std::copy(buf.data.Pointer(),
						  buf.data.Pointer() + buf.size,
						  outMemBegin + outMemPos);
				blockDesc.header.compBufferSizes[ids[i]] = buf.size;
				outMemPos += buf.size;
			}
		}

		// PPMd encoding start
		//
		for (uint32 i = DnaCompressedBin::PPMdStartBuffer; i <= DnaCompressedBin::PPMdEndBuffer; ++i)
		{
			uint64 inSize = dnaWorkBin_.buffers[i]->size;

			// take care of the case when we preallocated too small output memory
			if (inSize > 0)
			{
				byte* inMem = dnaWorkBin_.buffers[i]->data.Pointer();

				if (outMemPos + inSize > compBin_.dataBuffer.data.Size())
				{
					compBin_.dataBuffer.data.Extend(outMemPos + inSize, true);
					outMemBegin = compBin_.dataBuffer.data.Pointer();
				}

				byte* outMem = outMemBegin + outMemPos;
				unsigned long int outSize = compBin_.dataBuffer.data.Size() - outMemPos;

				bool r = ppmdEncoder->EncodeNextMember(inMem, inSize, outMem, outSize);

				ASSERT(r);
				ASSERT(outSize > 0);

				outMemPos += outSize;

				blockDesc.header.compBufferSizes[i] = outSize;
			}
			else
			{
				blockDesc.header.compBufferSizes[i] = 0;
			}
		}
		//
		// PPMd end


		// write header
		//
		blockWriter.SetPosition(0);
		blockWriter.Put4Bytes(blockDesc.header.minimizerId);
		blockWriter.Put8Bytes(blockDesc.header.recordsCount);
		blockWriter.PutByte(blockDesc.header.recMinLen);
		blockWriter.PutByte(blockDesc.header.recMaxLen);
		blockWriter.Put8Bytes(blockDesc.header.rawDnaStreamSize);

		// save work buffer sizes
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
			blockWriter.Put8Bytes(blockDesc.header.workBufferSizes[i]);

		// save PPMd buffer sizes
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
		{
			blockWriter.Put8Bytes(blockDesc.header.compBufferSizes[i]);

			compBin_.bufferSizes[i] = blockDesc.header.compBufferSizes[i];	// only for debug purposes
		}

		compBin_.dataBuffer.size = outMemPos;
	}


	// cleanup
	//
	CleanupWriters();
}


void DnaCompressor::CompressDnaRaw(DnaBin &dnaBin_, DnaCompressedBin& dnaWorkBin_, CompressedDnaBlock &compBin_)
{
	// just sort records for better matching
	//
	DnaRecordComparator comparator(0);
	std::sort(dnaBin_.Begin(), dnaBin_.End(), comparator);


	// prepare the work and output buffer buffer
	//
	const uint32 bitsPerLen = blockDesc.isLenConst ? 0 : bit_length(blockDesc.header.recMaxLen - blockDesc.header.recMinLen);
	const uint64 workPreallocSize = blockDesc.header.recordsCount * blockDesc.header.recMaxLen;
	const uint64 outPreallocSize = workPreallocSize
									+ (DnaShortBlockHeader::Size + 2*8)
									+ ((bitsPerLen > 0) ? blockDesc.header.recordsCount * bitsPerLen : 0);

	DataChunk& workBuffer = *dnaWorkBin_.buffers[DnaCompressedBin::HardReadsBuffer];
	if (workBuffer.data.Size() < workPreallocSize)
		workBuffer.data.Extend(workPreallocSize);
	workBuffer.size = 0;

	if (compBin_.dataBuffer.data.Size() < outPreallocSize)
		compBin_.dataBuffer.data.Extend(outPreallocSize);
	std::fill(compBin_.bufferSizes, compBin_.bufferSizes + DnaCompressedBin::BuffersNum, 0);
	compBin_.dataBuffer.size = 0;


	// prepare output writer and reserve header space
	//
	BitMemoryWriter blockWriter(compBin_.dataBuffer.data);
	blockWriter.FillBytes(0, DnaShortBlockHeader::Size);
	blockWriter.Put8Bytes(0); // comp size


	// copy data to work buffer and fill the lengths in case of variable lengths
	//
	for (uint64 i = 0; i < dnaBin_.Size(); ++i)
	{
		DnaRecord& r = dnaBin_[i];
		std::copy(r.dna, r.dna + r.len, workBuffer.data.Pointer() + workBuffer.size);
		workBuffer.size += r.len;

		if (!blockDesc.isLenConst)
			blockWriter.PutBits(r.len - blockDesc.header.recMinLen, bitsPerLen);
	}

	ASSERT(workBuffer.size == blockDesc.header.rawDnaStreamSize);

	if (!blockDesc.isLenConst)
	{
		blockWriter.FlushPartialWordBuffer();
		compBin_.bufferSizes[DnaCompressedBin::LenBuffer] = blockWriter.Position() - (DnaShortBlockHeader::Size + 1*8);	// TODO: make constant
	}


	const uint64 dnaDataOffset = blockWriter.Position();


	// compress PPMd
	//
	{
		byte* inMem = workBuffer.data.Pointer();
		uint64 inSize = workBuffer.size;

		byte* outMem = compBin_.dataBuffer.data.Pointer() + dnaDataOffset;
		unsigned long int outSize = compBin_.dataBuffer.data.Size() - dnaDataOffset;
		ASSERT(outSize >= inSize);

		bool r = ppmdEncoder->EncodeNextMember(inMem, inSize, outMem, outSize);

		ASSERT(r);
		ASSERT(outSize > 0);

		// write header
		//
		blockWriter.SetPosition(0);
		blockWriter.Put4Bytes(blockDesc.header.minimizerId);
		blockWriter.Put8Bytes(blockDesc.header.recordsCount);
		blockWriter.PutByte(blockDesc.header.recMinLen);
		blockWriter.PutByte(blockDesc.header.recMaxLen);
		blockWriter.Put8Bytes(blockDesc.header.rawDnaStreamSize);

		blockWriter.Put8Bytes(outSize);	// comp size

		compBin_.dataBuffer.size = outSize + dnaDataOffset;

		compBin_.bufferSizes[DnaCompressedBin::HardReadsBuffer] = outSize;
	}
}

void DnaCompressor::CompressRecordNormal(const DnaRecord &rec_)
{
	int32 minPos = rec_.minimizerPos;

	// pop last element in cyclic buffer
	LzMatch* newLz = prevBuffer.back();
	prevBuffer.pop_back();

	// get the best match
	MatchResult matchResult = FindBestLzMatch(rec_, minPos);
	LzMatch* bestLz = prevBuffer[matchResult.prevId];

	// prepare new match
	newLz->seq = rec_.dna;
	newLz->seqLen = rec_.len;
	newLz->minPos = minPos;

	bool identicalReads = (matchResult.prevId == 0 && matchResult.cost == 0 && bestLz->seqLen == rec_.len);
	bool isReadDifficult = (matchResult.cost >= (compParams.encodeThresholdValue != 0 ? compParams.encodeThresholdValue : rec_.len / 2) );

	// if the read is the same as previous, just push it back (might just have been non-pop'ped back previously)
	ASSERT(std::search(newLz->seq, newLz->seq + newLz->seqLen, currentMinimizerBuf,
					   currentMinimizerBuf + minParams.signatureSuffixLen) != newLz->seq + newLz->seqLen);
	if (identicalReads)
		prevBuffer.push_back(newLz);
	else
		prevBuffer.push_front(newLz);

	revCoder->coder.EncodeSymbol(revCoder->rc, (uint32)rec_.reverse);

	if (identicalReads)				// just store flag indicating that reads are equal
	{
		flagCoder->coder.EncodeSymbol(flagCoder->rc, ReadIdentical);
	}
	else if (isReadDifficult)		// perform full encoding of the read
	{
		flagCoder->coder.EncodeSymbol(flagCoder->rc, ReadDifficult);

		if (!blockDesc.isLenConst)
			writers[DnaCompressedBin::LenBuffer]->PutByte(newLz->seqLen);

		for (int32 i = 0; i < newLz->seqLen; ++i)
		{
			if (i < newLz->minPos || i >= newLz->minPos + minParams.signatureSuffixLen)
				writers[DnaCompressedBin::HardReadsBuffer]->PutByte(newLz->seq[i]);
			else if (i == newLz->minPos)
				writers[DnaCompressedBin::HardReadsBuffer]->PutByte(MinimizerPositionSymbol);
		}
	}
	else							// perform dfferential encoding of the read
	{
		if (!blockDesc.isLenConst)
			writers[DnaCompressedBin::LenBuffer]->PutByte(newLz->seqLen);

		writers[DnaCompressedBin::ShiftBuffer]->PutByte((int32)(ShiftOffset + matchResult.shift));
		writers[DnaCompressedBin::LzIdBuffer]->PutByte(matchResult.prevId);

		const char* bestSeq = bestLz->seq;
		int32 bestLen = bestLz->seqLen;
		int32 bestPos = bestLz->minPos;

		const char* newSeq = newLz->seq;
		int32 newLen = newLz->seqLen;

		if (matchResult.shift >= 0)
		{
			bestSeq += matchResult.shift;
			bestLen -= matchResult.shift;
			bestPos -= matchResult.shift;
		}
		else
		{
			// encode insertion
			for (int32 i = 0; i < -matchResult.shift; ++i)
				lettersCoder->coder.EncodeSymbol(lettersCoder->rc, dnaToIdx[(int32)newLz->seq[i]], dnaToIdx[(int32)'N']);

			newSeq += -matchResult.shift;
			newLen -= -matchResult.shift;
#if EXP_USE_RC_ADV
			ASSERT(bestPos - matchResult.shift < rec_.len - (rec_.reverse ? 0 : minParams.skipZoneLen));
#else
			ASSERT(bestPos - matchResult.shift < rec_.len - minParams.skipZoneLen);
#endif
		}
		int32 minLen = MIN(bestLen, newLen);

		// check whether the strings differ only at last position
		bool onlyLastSymDifference = (bestSeq[minLen-1] != newSeq[minLen-1]);		// this might differ from original algorithm
		if (onlyLastSymDifference)
		{
			// TODO: use std::equal / strcmpn
			for (int32 i = 0; i < minLen - 1; ++i)
				onlyLastSymDifference &= (bestSeq[i] == newSeq[i]);
		}

		uint32 flag;
		if (matchResult.noMismatches)
			flag = ReadShiftOnly;
		else if (onlyLastSymDifference)
			flag = ReadLastPosDifference;
		else
			flag = ReadFullEncode;

		flagCoder->coder.EncodeSymbol(flagCoder->rc, flag);

		if (flag == ReadFullEncode)
		{
			// TODO: optimize this routine
			for (int32 i = 0; i < minLen; ++i)
			{
				if (i == bestPos)
				{
					i += minParams.signatureSuffixLen - 1;
					continue;
				}

				if (bestSeq[i] == newSeq[i])
					rleEncoder->PutSymbol(true);
				else
				{
					rleEncoder->PutSymbol(false);
					lettersCoder->coder.EncodeSymbol(lettersCoder->rc, dnaToIdx[(int32)newSeq[i]], dnaToIdx[(int32)bestSeq[i]]);
				}
			}
		}
		else if (flag == ReadLastPosDifference)
		{
			lettersCoder->coder.EncodeSymbol(lettersCoder->rc, dnaToIdx[(int32)newSeq[minLen-1]], dnaToIdx[(int32)bestSeq[minLen-1]]);
		}

		// encode insertion
		for (int32 i = minLen; i < newLen; ++i)
			lettersCoder->coder.EncodeSymbol(lettersCoder->rc, dnaToIdx[(int32)newSeq[i]], dnaToIdx[(int32)'N']);
	}
}


DnaDecompressor::DnaDecompressor(const MinimizerParameters &minParams_)
	:	DnaStoreBase(minParams_)
{
	// TODO: refactor -- we can initialize all readers and encoders in ctor
	//
	ppmdDecoder = new PpmdDecoder();
	ppmdDecoder->StartDecompress(PpmdMemorySizeMb);
}

DnaDecompressor::~DnaDecompressor()
{
	CleanupReaders();

	ppmdDecoder->FinishDecompress();
	delete ppmdDecoder;
}

void DnaDecompressor::PrepareReaders(DnaCompressedBin &dnaWorkBin_)
{
	ASSERT(readers.size() == 0);

	for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
		readers.push_back(new BitMemoryReader(dnaWorkBin_.buffers[i]->data, dnaWorkBin_.buffers[i]->size));

	flagCoder = new FlagDecoder(*readers[DnaCompressedBin::FlagBuffer]);
	revCoder = new RevDecoder(*readers[DnaCompressedBin::RevBuffer]);
	lettersCoder = new LettersDecoder(*readers[DnaCompressedBin::LetterXBuffer]);
	rleDecoder = new BinaryRleDecoder(*readers[DnaCompressedBin::MatchBuffer]);
}

void DnaDecompressor::CleanupReaders()
{
	TFree(rleDecoder);
	TFree(lettersCoder);
	TFree(revCoder);
	TFree(flagCoder);

	for (uint32 i = 0; i < readers.size(); ++i)
		delete readers[i];
	readers.clear();
}


void DnaDecompressor::DecompressDna(CompressedDnaBlock &compBin_, DnaBin &dnaBin_, DnaCompressedBin& dnaWorkBin_, DataChunk& dnaBuffer_)
{
	// read header
	//
	blockDesc.Clear();
	BitMemoryReader blockReader(compBin_.dataBuffer.data, compBin_.dataBuffer.size);

	blockDesc.header.minimizerId = blockReader.Get4Bytes();
	blockDesc.header.recordsCount = blockReader.Get8Bytes();
	blockDesc.header.recMinLen = blockReader.GetByte();
	blockDesc.header.recMaxLen = blockReader.GetByte();
	blockDesc.header.rawDnaStreamSize = blockReader.Get8Bytes();
	blockDesc.isLenConst = (blockDesc.header.recMinLen == blockDesc.header.recMaxLen);

	ASSERT(blockDesc.header.recordsCount > 0);
	ASSERT(blockDesc.header.recMaxLen > 0);
	ASSERT(blockDesc.header.recMaxLen >= blockDesc.header.recMinLen);
	ASSERT(blockReader.Position() == DnaShortBlockHeader::Size);
	ASSERT(blockDesc.header.rawDnaStreamSize > 0);


	// prepare dna bin
	//
	dnaBin_.Resize(blockDesc.header.recordsCount);

	if (dnaBuffer_.data.Size() < blockDesc.header.rawDnaStreamSize)
		dnaBuffer_.data.Extend(blockDesc.header.rawDnaStreamSize);

	if (blockDesc.header.minimizerId != minParams.TotalMinimizersCount())
		DecompressDnaFull(compBin_, dnaWorkBin_, dnaBin_, dnaBuffer_);
	else
		DecompressDnaRaw(compBin_, dnaWorkBin_, dnaBin_, dnaBuffer_);

	ASSERT(dnaBuffer_.size == blockDesc.header.rawDnaStreamSize);
}

void DnaDecompressor::DecompressDnaFull(CompressedDnaBlock &compBin_, DnaCompressedBin& dnaWorkBin_, DnaBin &dnaBin_, DataChunk& dnaBuffer_)
{
	BitMemoryReader blockReader(compBin_.dataBuffer.data, compBin_.dataBuffer.size);
	blockReader.SetPosition(DnaShortBlockHeader::Size);

	// prepare work buffers
	{
		// get work buffer sizes
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
			blockDesc.header.workBufferSizes[i] = blockReader.Get8Bytes();

		// get compressed buffer sizes
		uint64 totalSize = DnaBlockHeader::Size;
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
		{
			blockDesc.header.compBufferSizes[i] = blockReader.Get8Bytes();
			totalSize += blockDesc.header.compBufferSizes[i];
		}

		ASSERT(totalSize == compBin_.dataBuffer.size);
	}


	// prepare buffers
	//
	{
		for (uint32 i = 0; i < DnaCompressedBin::BuffersNum; ++i)
		{
			if (dnaWorkBin_.buffers[i]->data.Size() < blockDesc.header.workBufferSizes[i])
				dnaWorkBin_.buffers[i]->data.Extend(blockDesc.header.workBufferSizes[i] + (blockDesc.header.workBufferSizes[i] / 8));
		}
	}

	// PPMd decompress
	//
	{
		byte* inMemBegin = compBin_.dataBuffer.data.Pointer();
		uint64 inMemPos = blockReader.Position();

		// copy range-coded buffers
		{
			uint32 ids[] = {
							DnaCompressedBin::FlagBuffer,
							DnaCompressedBin::RevBuffer,
							DnaCompressedBin::LetterXBuffer
						  };

			uint32 elems = sizeof(ids) / sizeof(uint32);
			for (uint32 i = 0; i < elems; ++i)
			{
				const uint64 size = blockDesc.header.workBufferSizes[ids[i]];
				DataChunk& buf = *dnaWorkBin_.buffers[ids[i]];

				if (buf.data.Size() < size)
					buf.data.Extend(size);

				std::copy(inMemBegin + inMemPos,
						  inMemBegin + inMemPos + size,
						  buf.data.Pointer());

				dnaWorkBin_.buffers[ids[i]]->size = size;
				inMemPos += size;
			}
		}


		// PPMd start
		//
		for (uint32 i = DnaCompressedBin::PPMdStartBuffer; i <= DnaCompressedBin::PPMdEndBuffer; ++i)
		{
			uint64 inSize = blockDesc.header.compBufferSizes[i];

			if (inSize > 0)
			{
				byte* inMem = inMemBegin + inMemPos;

				unsigned long int outSize = dnaWorkBin_.buffers[i]->data.Size();
				byte* outMem = dnaWorkBin_.buffers[i]->data.Pointer();

				bool r = ppmdDecoder->DecodeNextMember(inMem, inSize, outMem, outSize);
				ASSERT(r);
				ASSERT(outSize > 0);
				inMemPos += inSize;

				ASSERT(blockDesc.header.workBufferSizes[i] == outSize);
				dnaWorkBin_.buffers[i]->size = outSize;
			}
			else
			{
				dnaWorkBin_.buffers[i]->size = 0;
			}
		}
		//
		// PPMd end
	}

	// prepare minimizer config + lz
	//
	minParams.GenerateMinimizer(blockDesc.header.minimizerId, currentMinimizerBuf);
	currentMinimizerBuf[minParams.signatureSuffixLen] = 0;

	PrepareLzBuffer(LzBufferSize + 1);


	// prepare decoders
	//
	PrepareReaders(dnaWorkBin_);

	rleDecoder->Start();
	flagCoder->Start();
	revCoder->Start();
	lettersCoder->Start();


	// decode records
	//
	char* dnaBufferPtr = (char*)dnaBuffer_.data.Pointer();
	uint64 dnaBuferPos = 0;

	for (uint32 i = 0; i < dnaBin_.Size(); ++i)
	{
		DnaRecord& rec = dnaBin_[i];
		rec.dna = dnaBufferPtr + dnaBuferPos;
		DecompressRecordNormal(rec);

		dnaBuferPos += rec.len;

		ASSERT(dnaBuferPos <= dnaBuffer_.data.Size());
	}

	// finish decoding
	//
	dnaBuffer_.size = dnaBuferPos;

	rleDecoder->End();
	flagCoder->End();
	revCoder->End();
	lettersCoder->End();


	// cleanup
	//
	CleanupReaders();
}

void DnaDecompressor::DecompressDnaRaw(CompressedDnaBlock &compBin_, DnaCompressedBin& /*dnaWorkBin_*/, DnaBin &dnaBin_, DataChunk& dnaBuffer_)
{
	BitMemoryReader blockReader(compBin_.dataBuffer.data, compBin_.dataBuffer.size);
	blockReader.SetPosition(DnaShortBlockHeader::Size);

	// read buffer sizes
	//
	const uint64 dnaBufferSize = blockDesc.header.rawDnaStreamSize;
	const uint64 ppmdBufferSize = blockReader.Get8Bytes();

	// prepare output bin -- just raw reads
	//
	dnaBin_.Resize(blockDesc.header.recordsCount);


	// read records lengths in case of variable lengths -- TODO: optimize, move reading of lengths down
	//
	if (!blockDesc.isLenConst)
	{
		uint32 bitsPerLen = bit_length(blockDesc.header.recMaxLen - blockDesc.header.recMinLen);

		for (uint32 i = 0; i < dnaBin_.Size(); ++i)
		{
			DnaRecord& r = dnaBin_[i];
			r.len = blockDesc.header.recMinLen + blockReader.GetBits(bitsPerLen);
		}
		blockReader.FlushInputWordBuffer();
	}
	const uint64 dnaDataOffset = blockReader.Position();


	// PPMD decompress
	//
	{
		uint64 inSize = ppmdBufferSize;
		byte* inMem = compBin_.dataBuffer.data.Pointer() + dnaDataOffset;
		unsigned long int outSize = dnaBufferSize;
		byte* outMem = dnaBuffer_.data.Pointer();

		bool r = ppmdDecoder->DecodeNextMember(inMem, inSize, outMem, outSize);
		ASSERT(r);
		ASSERT(outSize > 0);
		ASSERT(dnaBufferSize == outSize);
	}

	// decode records
	//
	char* dnaBufferPtr = (char*)dnaBuffer_.data.Pointer();
	uint64 bufferPos = 0;
	for (uint32 i = 0; i < dnaBin_.Size(); ++i)
	{
		DnaRecord& r = dnaBin_[i];
		r.reverse = false;
		r.minimizerPos = 0;				// not needed, TODO: remove me

		if (blockDesc.isLenConst)
			r.len = blockDesc.header.recMinLen;
		ASSERT(bufferPos + r.len <= dnaBufferSize);

		r.dna = dnaBufferPtr + bufferPos;
		bufferPos += r.len;
	}

	ASSERT(bufferPos == dnaBufferSize);
	dnaBuffer_.size = bufferPos;
}


void DnaDecompressor::DecompressRecordNormal(DnaRecord &rec_)
{
	uint32 flag = flagCoder->coder.DecodeSymbol(flagCoder->rc);
	ASSERT(flag < 5);

	rec_.reverse = revCoder->coder.DecodeSymbol(revCoder->rc) != 0;

	LzMatch* nextLz = prevBuffer.back();
	prevBuffer.pop_back();

	if (flag == ReadIdentical)				// 	read same as the previous one
	{
		LzMatch* m = prevBuffer.front();
		std::copy(m->seq, m->seq + m->seqLen, rec_.dna);
		rec_.len = m->seqLen;

		// prepare next LZ
		nextLz->seq = m->seq;
		nextLz->seqLen = m->seqLen;
	}
	else if (flag == ReadDifficult)			// reads are too different - full encoding
	{
		if (!blockDesc.isLenConst)
			rec_.len = readers[DnaCompressedBin::LenBuffer]->GetByte();
		else
			rec_.len = blockDesc.header.recMinLen;
		ASSERT(rec_.len > 0);
		ASSERT(rec_.len < 255);

		rec_.minimizerPos = 0;
		for (int32 i = 0; i < rec_.len; ++i)
		{
			int32 c = readers[DnaCompressedBin::HardReadsBuffer]->GetByte();
			if (c != MinimizerPositionSymbol)
			{
				ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
				rec_.dna[i] = c;
			}
			else
			{
				std::copy(currentMinimizerBuf, currentMinimizerBuf + minParams.signatureSuffixLen, rec_.dna + i);
				rec_.minimizerPos = i;
				i += minParams.signatureSuffixLen - 1;
			}
		}

		// prepare next LZ
		nextLz->seq = rec_.dna;
		nextLz->seqLen = rec_.len;
		nextLz->minPos = rec_.minimizerPos;
	}
	else									// reads are similar - mismatch coding
	{
		if (!blockDesc.isLenConst)
			rec_.len = readers[DnaCompressedBin::LenBuffer]->GetByte();
		else
			rec_.len = blockDesc.header.recMinLen;

		ASSERT(rec_.len > 0);
		ASSERT(rec_.len < 255);

		int32 recLen = rec_.len;
		char* recStr = rec_.dna;
		int32 shift = (int32)(readers[DnaCompressedBin::ShiftBuffer]->GetByte()) - ShiftOffset;
		ASSERT(shift < recLen - minParams.signatureSuffixLen);

		int32 prevId = readers[DnaCompressedBin::LzIdBuffer]->GetByte();

		LzMatch* bestLz = prevBuffer[prevId];
		const char* bestSeq = bestLz->seq;
		int32 bestLen = bestLz->seqLen;
		int32 bestPos = bestLz->minPos;

		int32 nextMinPos = 0;

		if (shift >= 0)
		{
			bestSeq += shift;
			bestLen -= shift;
			bestPos -= shift;
			nextMinPos = bestPos;
		}
		else
		{
			for (int32 i = 0; i < -shift; ++i)
			{
				int32 c = lettersCoder->coder.DecodeSymbol(lettersCoder->rc, dnaToIdx['N']);
				ASSERT(c < 5);
				c = idxToDna[c];

				ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
				rec_.dna[i] = c;
			}

			recStr += -shift;
			recLen -= -shift;
			nextMinPos = bestPos + (-shift);
		}

#if EXP_USE_RC_ADV
		ASSERT(nextMinPos < rec_.len - (rec_.reverse ? 0 : minParams.skipZoneLen));
#else
		ASSERT(nextMinPos < rec_.len - minParams.skipZoneLen);
#endif

		int32 minLen = MIN(bestLen, recLen);
		ASSERT(minLen > 0);

		if (flag == ReadFullEncode)
		{
			// TODO: optimize this routine
			for (int32 i = 0; i < minLen; ++i)
			{
				if (i == bestPos)
				{
					std::copy(currentMinimizerBuf, currentMinimizerBuf + minParams.signatureSuffixLen, recStr + i);
					i += minParams.signatureSuffixLen - 1;
					continue;
				}

				if (rleDecoder->GetSym())
				{
					recStr[i] = bestSeq[i];
				}
				else
				{
					int32 c = lettersCoder->coder.DecodeSymbol(lettersCoder->rc, dnaToIdx[(int32)bestSeq[i]]);
					ASSERT(c < 5);
					c = idxToDna[c];

					ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
					recStr[i] = c;
				}
			}
		}
		else if (flag == ReadLastPosDifference)
		{
			// TODO: use std::copy
			for (int32 i = 0; i < minLen - 1; ++i)
				recStr[i] = bestSeq[i];

			int32 c = lettersCoder->coder.DecodeSymbol(lettersCoder->rc, dnaToIdx[(int32)bestSeq[minLen-1]]);
			ASSERT(c < 5);
			c = idxToDna[c];

			ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
			recStr[minLen-1] = c;
		}
		else					// all symbols match
		{
			// TODO: use std::copy
			for (int32 i = 0; i < minLen; ++i)
				recStr[i] = bestSeq[i];
		}

		for (int32 i = minLen; i < recLen; ++i)
		{
			int32 c = lettersCoder->coder.DecodeSymbol(lettersCoder->rc, dnaToIdx['N']);
			ASSERT(c < 5);
			c = idxToDna[c];

			ASSERT(c == 'A' || c == 'C' || c == 'G' || c == 'T' || c == 'N');
			recStr[i] = c;
		}

		// prepare next lz
		nextLz->seq = rec_.dna;
		nextLz->seqLen = rec_.len;
		nextLz->minPos = nextMinPos;
	}

	ASSERT(std::search(nextLz->seq, nextLz->seq + nextLz->seqLen, currentMinimizerBuf,
					   currentMinimizerBuf + minParams.signatureSuffixLen) != nextLz->seq + nextLz->seqLen);
	ASSERT(std::search(rec_.dna, rec_.dna + rec_.len, currentMinimizerBuf,
					   currentMinimizerBuf + minParams.signatureSuffixLen) != rec_.dna + rec_.len);

	// add only unique reads to lz table
	if (flag != ReadIdentical)
		prevBuffer.push_front(nextLz);
	else
		prevBuffer.push_back(nextLz);
}

