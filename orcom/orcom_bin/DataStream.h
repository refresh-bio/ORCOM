/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_DATASTREAM
#define H_DATASTREAM

#include "Globals.h"


class IDataStream
{
public:
	virtual ~IDataStream() {}

	virtual void Close() = 0;

	virtual uint64 Size() const = 0;
	virtual uint64 Position() const = 0;
	virtual void SetPosition(uint64 pos_) = 0;
};


class IDataStreamReader : public IDataStream
{
public:
	virtual int64 Read(uchar* mem_, uint64 size_) = 0;
};


class IDataStreamWriter : public IDataStream
{
public:
	virtual	int64 Write(const uchar* mem_, uint64 size_) = 0;
};


#endif // H_DATASTREAM
