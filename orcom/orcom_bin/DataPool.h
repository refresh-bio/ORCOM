#ifndef DATAPOOL_H
#define DATAPOOL_H

#include "Globals.h"

#include <vector>

#include "Thread.h"

template <class _TDataType>
class TDataPool
{
	typedef _TDataType DataType;
	typedef std::vector<DataType*> part_pool;

	const uint32 maxPartNum;
	const uint32 bufferPartSize;
	uint32 partNum;

	part_pool availablePartsPool;
	part_pool allocatedPartsPool;

	mt::mutex mutex;
	mt::condition_variable partsAvailableCondition;


public:
	static const uint32 DefaultMaxPartNum = 32;
	static const uint32 DefaultBufferPartSize = 1 << 22;

	TDataPool(uint32 maxPartNum_ = DefaultMaxPartNum, uint32 bufferPartSize_ = DefaultBufferPartSize, uint32 preAllocateSize_ = 0)
		:	maxPartNum(maxPartNum_)
		,	bufferPartSize(bufferPartSize_)
		,	partNum(0)
	{
		availablePartsPool.resize(maxPartNum);
		allocatedPartsPool.reserve(maxPartNum);

		for (uint32 i = 0; i < preAllocateSize_; ++i)
		{
			DataType* pp = new DataType(bufferPartSize);
			allocatedPartsPool.push_back(pp);

			availablePartsPool[availablePartsPool.size() - i - 1] = pp;
		}

	}

	virtual ~TDataPool()
	{
		for (typename part_pool::iterator i = allocatedPartsPool.begin(); i != allocatedPartsPool.end(); ++i)
		{
			ASSERT(*i != NULL);
			delete *i;
		}
	}

	virtual void Acquire(DataType* &part_)
	{
		mt::unique_lock<mt::mutex> lock(mutex);

		while (partNum >= maxPartNum)
			partsAvailableCondition.wait(lock);

		ASSERT(availablePartsPool.size() > 0);

		DataType*& pp = availablePartsPool.back();
		availablePartsPool.pop_back();
		if (pp == NULL)
		{
			pp = new DataType(bufferPartSize);
			allocatedPartsPool.push_back(pp);
		}
		else
		{
			pp->Reset();
		}

		partNum++;
		part_ = pp;
	}

	virtual void Release(const DataType* part_)
	{
		mt::lock_guard<mt::mutex> lock(mutex);

		ASSERT(part_ != NULL);
		ASSERT(partNum != 0 && partNum <= maxPartNum);
		ASSERT(std::find(allocatedPartsPool.begin(), allocatedPartsPool.end(), part_) != allocatedPartsPool.end());

		availablePartsPool.push_back((DataType*)part_);
		partNum--;

		partsAvailableCondition.notify_one();
	}
};


#endif // DATAPOOL_H
