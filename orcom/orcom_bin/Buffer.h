#ifndef BUFFER_H
#define BUFFER_H

#include "Globals.h"

#include <algorithm>

#include "Utils.h"

class IBuffer
{
public:
	IBuffer(byte* ptr_, uint64 size_)
		:	buffer(ptr_)
		,	size(size_)
	{}

	virtual ~IBuffer() {}

	uint64 Size() const
	{
		return size;
	}

	byte* Pointer() const
	{
		return (byte*)buffer;
	}

	uint64* Pointer64() const
	{
		return (uint64*)buffer;
	}

	void SetSize(uint64 s_)
	{
		size = s_;
	}

	void SetPointer(byte* ptr_)
	{
		buffer = ptr_;
	}

protected:
	byte* buffer;
	uint64 size;

private:
	IBuffer(const IBuffer& )
	{}

	IBuffer& operator= (const IBuffer& )
	{ return *this; }
};


class Buffer : public IBuffer
{
public:
	Buffer(uint64 size_)
		:	IBuffer(Alloc(size_), size_)
	{
		ASSERT(size_ != 0);
	}

	~Buffer()
	{
		delete buffer;
	}

	void Extend(uint64 size_, bool copy_ = false)
	{		

		uint64 size64 = size / 8;
		if (size64 * 8 < size)
			size64 += 1;

		uint64 newSize64 = size_ / 8;
		if (newSize64 * 8 < size_)
			newSize64 += 1;

		if (size > size_)
			return;

		if (size64 == newSize64)
		{
			size = size_;
			return;
		}

		uint64* p = new uint64[newSize64];

		if (copy_)
			std::copy(buffer, buffer + size, (byte*)p);

		delete[] buffer;

		buffer = (byte*)p;
		size = size_;
	}

	void Swap(Buffer& b)
	{
		TSwap(b.buffer, buffer);
		TSwap(b.size, size);
	}

	static byte* Alloc(uint64 size_)
	{
		uint64 size64 = size_ / 8;
		if (size64 * 8 < size_)
			size64 += 1;
		return (byte*)(new uint64[size64]);
	}

private:
	Buffer(const Buffer& ) : IBuffer(NULL, 0)
	{}

	Buffer& operator= (const Buffer& )
	{ return *this; }
};


struct DataChunk
{
	static const uint64 DefaultBufferSize = 1 << 20;		// 1 << 22

	Buffer data;
	uint64 size;

	DataChunk(uint64 bufferSize_ = DefaultBufferSize)
		:	data(bufferSize_)
		,	size(0)
	{}

	void Reset()
	{
		size = 0;
	}
};


#endif // BUFFER_H
