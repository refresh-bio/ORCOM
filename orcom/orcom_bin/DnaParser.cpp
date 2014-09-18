#include "DnaParser.h"
#include "DnaBlockData.h"

#define REC_EXTENSION_FACTOR(size)		( ((size) / 4 > 1024) ? ((size) / 4) : 1024 )


DnaParser::DnaParser()
	:	memory(NULL)
	,	memoryPos(0)
	,	memorySize(0)
	,	skippedBytes(0)
	,	buf(NULL)
	,	dnaMemory(NULL)
	,	dnaSize(0)
{
	revRecord.dna = revBuffer;
}

uint64 DnaParser::ParseFrom(const DataChunk& chunk_, DataChunk& dnaBuffer_, std::vector<DnaRecord>& records_, uint64& rec_count_)
{
	memory = chunk_.data.Pointer();
	memoryPos = 0;
	memorySize = chunk_.size;

	if (dnaBuffer_.data.Size() < chunk_.size / 2)
		dnaBuffer_.data.Extend(chunk_.size / 2);
	dnaMemory = dnaBuffer_.data.Pointer();
	dnaSize = 0;

	rec_count_ = 0;
	while (memoryPos < memorySize && ReadNextRecord(records_[rec_count_]))
	{
		rec_count_++;
		if (records_.size() < rec_count_ + 1)
			records_.resize(records_.size() + REC_EXTENSION_FACTOR(records_.size()));
	}
	ASSERT(rec_count_ > 0);
	ASSERT(dnaSize > 0);

	dnaBuffer_.size = dnaSize;

	return chunk_.size - skippedBytes;
}


bool DnaParser::ReadNextRecord(DnaRecord& rec_)
{
	if (memoryPos == memorySize)
		return false;

	const char* title = (const char*)(memory + memoryPos);
	uint32 tlen = SkipLine();
	if (tlen == 0 || title[0] != '@')
		return false;

	const char* seq = (const char*)(memory + memoryPos);
	uint32 slen = SkipLine();

	uint32 plen = SkipLine();
	if (plen == 0)
		return false;

	uint32 qlen = SkipLine();
	if (qlen != slen)
		return false;

	ASSERT(slen < DnaRecord::MaxDnaLen);


	std::copy(seq, seq + slen, dnaMemory + dnaSize);

	rec_.dna = (char*)(dnaMemory + dnaSize);
	dnaSize += slen;

	rec_.len = slen;
	rec_.reverse = false;

	return true;
}

bool DnaParser::ReadLine(uchar *str_, uint32& len_, uint32& size_)
{
	uint32 i = 0;
	for (;;)
	{
		int32 c = Getc();
		if (c == -1)
			break;

		if (c != '\n' && c != '\r')
		{
			if (i >= size_)
			{
				extend_string(str_, size_);
			}
			str_[i++] = (uchar)c;
		}
		else
		{
			if (c == '\r' && Peekc() == '\n')	// case of CR LF
				Skipc();

			if (i > 0)
				break;
		}
	}
	str_[i] = 0;
	len_ = i;
	return i > 0;
}

uint32 DnaParser::SkipLine()
{
	uint32 len = 0;
	for (;;)
	{
		int32 c = Getc();
		if (c == -1)
			break;

		if (c != '\n' && c != '\r')
		{
			len++;
		}
		else
		{
			if (c == '\r' && Peekc() == '\n')	// case of CR LF
				Skipc();

			break;
		}
	}
	return len;
}

uint64 DnaParser::ParseTo(const DnaBinBlock &dnaBins_, DataChunk &chunk_)
{
	buf = &chunk_.data;
	memory = buf->Pointer();
	memoryPos = 0;
	memorySize = chunk_.data.Size();

	for (uint32 binId = 0; binId < dnaBins_.stdBins.Size(); ++binId)
	{
		const DnaBin& dnaBin = dnaBins_.stdBins[binId];

		for (uint32 r = 0; r < dnaBin.Size(); ++r)
			WriteNextRecord(dnaBin[r]);
	}

	for (uint32 r = 0; r < dnaBins_.nBin.Size(); ++r)
		WriteNextRecord(dnaBins_.nBin[r]);

	chunk_.size = memoryPos;
	return memoryPos;
}

void DnaParser::WriteNextRecord(const DnaRecord& rec_)
{
	ASSERT(rec_.len > 0);
	if (memoryPos + rec_.len + 1 > memorySize)
		ExtendBuffer();

	const char* dna = rec_.dna;

	if (rec_.reverse)
	{
		rec_.ComputeRC(revRecord);
		dna = revRecord.dna;
	}

	std::copy(dna, dna + rec_.len, memory + memoryPos);
	memoryPos += rec_.len;
	memory[memoryPos++] = '\n';
}

uint64 DnaParser::ParseTo(const DnaBin &dnaBin_, DataChunk &chunk_)
{
	buf = &chunk_.data;
	memory = buf->Pointer();
	memoryPos = 0;
	memorySize = chunk_.data.Size();

	for (uint32 r = 0; r < dnaBin_.Size(); ++r)
		WriteNextRecord(dnaBin_[r]);

	chunk_.size = memoryPos;
	return memoryPos;
}
