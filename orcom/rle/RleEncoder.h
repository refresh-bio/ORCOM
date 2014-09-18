#ifndef RLEENCODER_H
#define RLEENCODER_H

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/BitMemory.h"


class BinaryRleEncoder
{
public:
	BinaryRleEncoder(BitMemoryWriter& writer_)
		:	writer(writer_)
		,	currentCount(0)
	{}

	void PutSymbol(bool s_)
	{
		if (s_)
		{
			currentCount++;
			if(currentCount == RleMax - RleOffset)
			{
				Put(currentCount + RleOffset);
				currentCount = 0;
			}
		}
		else
		{
			bool mism = (currentCount > 0) && (currentCount < RleMax - RleOffset);
			if (currentCount > 0)
			{
				Put(currentCount + RleOffset);
				currentCount = 0;
			}
			if (!mism)
				Put(0);
		}
	}

	void Start()
	{
		currentCount = 0;
	}

	void End()
	{
		if (currentCount > 0)
		{
			Put(currentCount + RleOffset);
			currentCount = 0;
		}
	}


private:
	static const uint32 RleMax = 255;
	static const uint32 RleOffset = 2;

	BitMemoryWriter& writer;

	uint32 currentCount;

	void Put(uint32 s_)
	{
		writer.PutByte(s_);
	}
};


class BinaryRleDecoder
{
public:
	BinaryRleDecoder(BitMemoryReader& reader_)
		:	reader(reader_)
		,	currentCount(0)
		,	onlyMatches(false)
	{
	}

	void Start()
	{
		currentCount = 0;
		onlyMatches = false;
		Fetch();
	}

	void End()
	{

	}

	bool GetSym()
	{
		if (currentCount > RleOffset)
		{
			currentCount--;
			return true;
		}
		if (currentCount == 0 || (!onlyMatches && currentCount == RleOffset))
		{
			Fetch();
			return false;
		}

		Fetch();
		return GetSym();
	}

private:
	static const uint32 RleOffset = 2;
	static const uint32 RleMax = 255;

	BitMemoryReader& reader;
	uint32 currentCount;
	bool onlyMatches;

	void Fetch()
	{
		if (reader.Position() < reader.Size())
		{
			currentCount = reader.GetByte();
			onlyMatches = (currentCount == RleMax);
		}
	}
};

#endif // RLEENCODER_H
