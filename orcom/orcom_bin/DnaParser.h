/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_DNAPARSER
#define H_DNAPARSER

#include "Globals.h"

#include <vector>

#include "DnaRecord.h"
#include "Buffer.h"


class DnaParser
{
public:
	DnaParser();

	uint64 ParseTo(const DnaBinBlock& dnaBins_, DataChunk& chunk_);
	uint64 ParseTo(const DnaBin& dnaBin_, DataChunk& chunk_);

	uint64 ParseFrom(const DataChunk& chunk_, DataChunk& dnaBuffer_,
					 std::vector<DnaRecord>& records_, uint64& rec_count_);

protected:
	byte* memory;
	uint64 memoryPos;
	uint64 memorySize;
	uint64 skippedBytes;
	Buffer* buf;

	byte* dnaMemory;			// TODO: move out
	uint64 dnaSize;				// *
	DnaRecord revRecord;		// *
	char revBuffer[1024];		// * + make constant


	bool ReadNextRecord(DnaRecord& rec_);
	bool ReadLine(uchar *str_, uint32& len_, uint32& size_);
	uint32 SkipLine();
	void WriteNextRecord(const DnaRecord& rec_);


	int32 Getc()
	{
		if (memoryPos == memorySize)
			return -1;
		return memory[memoryPos++];
	}

	void Skipc()
	{
		memoryPos++;
		skippedBytes++;
	}

	int32 Peekc()
	{
		if (memoryPos == memorySize)
			return -1;
		return memory[memoryPos];
	}

	void ExtendBuffer()
	{
		ASSERT(buf != NULL);
		buf->Extend(buf->Size() + (buf->Size() >> 1), true);
		memory = buf->Pointer();
		memorySize = buf->Size();
	}
};


#endif // H_DNAPARSER
