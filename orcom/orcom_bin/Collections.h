/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_COLLECTION
#define H_COLLECTION

#include "Globals.h"

#include <vector>
#include <map>

#include "DnaRecord.h"


class ICollection
{
public:
	ICollection(uint64 size_ = 0)
		:	size(size_)
	{}

	virtual ~ICollection()
	{}

	uint64 Size() const
	{
		return size;
	}

	virtual void Reset()
	{}

protected:
	uint64 size;
};


template <class _T>
class TConstCollection : public ICollection
{
public:
	TConstCollection(uint64 size_ = 0)
		:	ICollection(size_)
	{
		if (size_ > 0)
			elems.resize(size_);
	}

	void Resize(uint64 size_)
	{
		if (elems.size() < size_)
			elems.resize(size_);
		size = size_;
	}

	const _T& operator[](uint64 i_) const
	{
		ASSERT(i_ < size);
		return elems[i_];
	}

	_T& operator[](uint64 i_)
	{
		ASSERT(i_ < size);
		return elems[i_];
	}

	typename std::vector<_T>::iterator Begin()
	{
		return elems.begin();
	}

	typename std::vector<_T>::iterator End()
	{
		return elems.begin() + size;
	}

protected:
	std::vector<_T> elems;
};


template <class _T>
class TConstPtrCollection : public ICollection
{
public:
	TConstPtrCollection(uint64 size_ = 0)
		:	ICollection(size_)
	{
		if (size_ > 0)
			elems.resize(size_);

		for (uint64 i = 0; i < size_; ++i)
			elems[i] = new _T();
	}

	~TConstPtrCollection()
	{
		for (uint64 i = 0; i < elems.size(); ++i)
		{
			ASSERT(elems[i] != NULL);
			delete elems[i];
		}
	}

	void Resize(uint64 size_)
	{
		if (elems.size() < size_)
		{
			for (uint64 i = elems.size(); i < size_; ++i)
				elems.push_back(new _T());
		}
		size = size_;
	}

	const _T& operator[](uint64 i_) const
	{
		ASSERT(i_ < size);
		return *elems[i_];
	}

	_T& operator[](uint64 i_)
	{
		ASSERT(i_ < size);
		return *elems[i_];
	}

protected:
	TConstPtrCollection(const TConstPtrCollection& )
	{}

	TConstPtrCollection& operator=(const TConstPtrCollection& )
	{
		return *this;
	}

	std::vector<_T*> elems;
};


struct DnaRecordStats
{
	uint32 minLen;
	uint32 maxLen;

	DnaRecordStats()
		:	minLen((uint32)-1)
		,	maxLen(0)
	{}
};


class DnaBin
{
public:
	DnaBin()
		:	size(0)
	{}

	void Insert(const DnaRecord& rec_)
	{
		ASSERT(rec_.len > 0);

		if (elems.size() <= size)
		{
			if (elems.size() == 0)
				elems.resize(MinimumBinSize);
			else if (elems.size() < ThresholdBinSize)
				elems.resize(size * 2);
			else
				elems.resize(size + (size / 4));
		}
		elems[size++] = rec_;

		/*
		if (elems.size() <= size)
			elems.push_back(rec_);
		else
			elems[size] = rec_;
		size++;
		*/

		UpdateStats(rec_);
	}

	uint64 Size() const
	{
		return size;
	}

	void Clear()
	{
		size = 0;
		ClearStats();
	}

	void Resize(uint64 size_)
	{
		if (elems.size() < size_)
			elems.resize(size_);
		size = size_;
	}

	const DnaRecord& operator[](uint64 i_) const
	{
		ASSERT(i_ < size);
		return elems[i_];
	}

	DnaRecord& operator[](uint64 i_)
	{
		ASSERT(i_ < size);
		return elems[i_];
	}

	const DnaRecordStats& GetStats() const
	{
		return stats;
	}

	void SetStats(uint32 minLen_, uint32 maxLen_)
	{
		stats.minLen = minLen_;
		stats.maxLen = maxLen_;
	}

	void UpdateStats(const DnaRecord& rec_)
	{
		stats.maxLen = MAX(stats.maxLen, rec_.len);
		stats.minLen = MIN(stats.minLen, rec_.len);
	}

	void ClearStats()
	{
		stats.maxLen = 0;
		stats.minLen = (uint32)-1;
	}

	std::vector<DnaRecord>::iterator Begin()
	{
		return elems.begin();
	}

	std::vector<DnaRecord>::iterator End()
	{
		return elems.begin() + size;
	}

private:
	static const uint32 MinimumBinSize = 32;
	static const uint32 ThresholdBinSize = 512;

	std::vector<DnaRecord> elems;
	uint64 size;

	DnaRecordStats stats;
};


class DnaBinCollection
{
public:
	typedef std::map<uint32, DnaBin> BinMap;
	typedef BinMap::iterator Iterator;
	typedef BinMap::const_iterator ConstIterator;

	DnaBinCollection()
	{}

	BinMap& GetBins()
	{
		return bins;
	}

	const BinMap& GetBins() const
	{
		return bins;
	}

	Iterator Begin()
	{
		return bins.begin();
	}

	Iterator End()
	{
		return bins.end();
	}

	ConstIterator CBegin() const
	{
		return bins.begin();
	}

	ConstIterator CEnd() const
	{
		return bins.end();
	}

	const DnaBin& operator[](uint32 i_) const
	{
		ASSERT(bins.count(i_) > 0);
		return bins.at(i_);
	}

	DnaBin& operator[](uint32 i_)
	{
		return bins[i_];
	}

	void Clear()
	{
		//bins.clear();

		for (BinMap::iterator i = bins.begin(); i != bins.end(); ++i)
			i->second.Clear();
	}

	bool Exists(uint32 id_) const
	{
		return bins.count(id_) > 0 && bins.at(id_).Size() > 0;
	}

private:
	BinMap bins;
};


#endif // H_COLLECTION
