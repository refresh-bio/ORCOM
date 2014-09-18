#ifndef H_FASTQSTREAM
#define H_FASTQSTREAM

#include "Globals.h"

#include <string>
#include <vector>

#include "FileStream.h"


class IFastqStreamReader
{
private:
	static const uint32 SwapBufferSize = 1 << 13;

public:
	IFastqStreamReader()
		:	stream(NULL)
		,	swapBuffer(SwapBufferSize)
		,	bufferSize(0)
		,	eof(false)
		,	usesCrlf(false)
	{}

	virtual ~IFastqStreamReader()
	{}

	bool Eof() const
	{
		return eof;
	}

	bool ReadNextChunk(DataChunk* chunk_);

	void Close()
	{
		ASSERT(stream != NULL);
		stream->Close();
	}

protected:
	IDataStreamReader* stream;

	int64 Read(byte* memory_, uint64 size_)
	{
		ASSERT(stream != NULL);
		return stream->Read(memory_, size_);
	}

private:
	Buffer			swapBuffer;
	uint64			bufferSize;
	bool			eof;
	bool			usesCrlf;

	uint64 GetNextRecordPos(uchar* data_, uint64 pos_, const uint64 size_);

	void SkipToEol(uchar* data_, uint64& pos_, const uint64 size_)
	{
		ASSERT(pos_ < size_);

		while (data_[pos_] != '\n' && data_[pos_] != '\r' && pos_ < size_)
			++pos_;

		if (data_[pos_] == '\r' && pos_ < size_)
		{
			if (data_[pos_ + 1] == '\n')
			{
				usesCrlf = true;
				++pos_;
			}
		}
	}
};

class IFastqStreamWriter
{
public:
	IFastqStreamWriter()
	{}

	virtual ~IFastqStreamWriter()
	{}

	void WriteNextChunk(const DataChunk* chunk_)
	{
		ASSERT(stream != NULL);
		stream->Write(chunk_->data.Pointer(), chunk_->size);
	}

	void Close()
	{
		ASSERT(stream != NULL);
		stream->Close();
	}

protected:
	IDataStreamWriter* stream;
};


// wrappers
//
class FastqFileReader : public IFastqStreamReader
{
public:
	FastqFileReader(const std::string& fileName_)
	{
		stream = new FileStreamReader(fileName_);
	}

	~FastqFileReader()
	{
		delete stream;
	}
};

class FastqFileWriter : public IFastqStreamWriter
{
public:
	FastqFileWriter(const std::string& fileName_)
	{
		stream = new FileStreamWriter(fileName_);
	}

	~FastqFileWriter()
	{
		delete stream;
	}
};

typedef FastqFileWriter DnaFileWriter;



class MultiFastqFileReader : public IFastqStreamReader
{
public:
	MultiFastqFileReader(const std::vector<std::string>& fileNames_)
	{
		stream = new MultiFileStreamReader(fileNames_);
	}

	~MultiFastqFileReader()
	{
		delete stream;
	}
};

#ifndef DISABLE_GZ_STREAM
class MultiFastqFileReaderGz : public IFastqStreamReader
{
public:
	MultiFastqFileReaderGz(const std::vector<std::string>& fileNames_)
	{
		stream = new MultiFileStreamReaderGz(fileNames_);
	}

	~MultiFastqFileReaderGz()
	{
		delete stream;
	}
};
#endif

#endif // H_FASTQSTREAM
