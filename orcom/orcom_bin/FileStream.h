#ifndef H_FILESTREAM
#define H_FILESTREAM

#include "Globals.h"

#include <string>
#include <vector>

#include "DataStream.h"
#include "Buffer.h"


class IFileStream
{
public:
	IFileStream();
	virtual ~IFileStream();

	void SetBuffering(bool enable_);

protected:
	struct FileStreamImpl;
	FileStreamImpl* impl;
};

class IMemoryStream
{
public:
	IMemoryStream();
	virtual ~IMemoryStream();

	void* Pointer();

protected:
	struct MemoryStreamImpl;
	MemoryStreamImpl* impl;
};


//
// readers
//
class FileStreamReader : public IDataStreamReader, public IFileStream
{
public:
	FileStreamReader(const std::string& fileName_);
	~FileStreamReader();

	void Close();

	void SetPosition(uint64 pos_);

	uint64 Position() const
	{
		return position;
	}

	uint64 Size() const
	{
		return size;
	}

	virtual int64 Read(uchar* mem_, uint64 size_);

private:
	uint64 size;
	uint64 position;
};

class MemoryStreamReader : public IDataStreamReader, public IMemoryStream
{
public:
	MemoryStreamReader(const std::string& fileName_);
	~MemoryStreamReader();

	int64 Read(uchar* mem_, uint64 size_);
	void Close();

	uint64 Size() const;
	void SetPosition(uint64 pos_);

	int64 Attach(uchar *&mem_, uint64 size_);

	IBuffer* MemoryBuffer() const
	{
		return memoryBuffer;
	}

	uint64 Position() const
	{
		return position;
	}

private:
	uint64 position;
	IBuffer* memoryBuffer;
};


struct IFileFuncImpl
{
	virtual ~IFileFuncImpl() {}
	virtual void* Open(const char* filename_, const char* flags_) const = 0;
	virtual void Close(void* file_) const = 0;
	virtual int64 Read(void* file_, byte* mem_, uint64 size_) const = 0;
};

class IMultiFileStreamReader : public IDataStreamReader, public IFileStream
{
public:
	IMultiFileStreamReader(const std::vector<std::string>& fileNames_, const IFileFuncImpl* fileFuncImpl_);
	~IMultiFileStreamReader();

	void Close();

	virtual int64 Read(uchar* mem_, uint64 size_);

	uint64 Size() const
	{
		return 0;
	}

	uint64 Position() const
	{
		return 0;
	}

	void SetPosition(uint64 )
	{
		ASSERT(0);
	}


protected:
	const std::vector<std::string>& fileNames;
	const IFileFuncImpl* fileFuncImpl;
	uint32 idx;

	bool OpenNext();
};


class MultiFileStreamReader : public IMultiFileStreamReader
{
public:
	MultiFileStreamReader(const std::vector<std::string>& fileNames_);
};

#ifndef DISABLE_GZ_STREAM
class MultiFileStreamReaderGz : public IMultiFileStreamReader
{
public:
	MultiFileStreamReaderGz(const std::vector<std::string>& fileNames_);
};
#endif




//
// writers
//
class FileStreamWriter : public IDataStreamWriter, public IFileStream
{
public:
	FileStreamWriter(const std::string& fileName_);
	~FileStreamWriter();

	void Close();

	virtual	int64 Write(const uchar* mem_, uint64 size_);

	void SetPosition(uint64 pos_);

	uint64 Position() const
	{
		return position;
	}

	uint64 Size() const
	{
		return 0;
	}

private:
	uint64 position;
};


#endif // H_FILESTREAM
