#ifndef DNARCHFILE_H
#define DNARCHFILE_H

#include "../orcom_bin/Globals.h"

#include "Params.h"
#include "CompressedBlockData.h"

#include "../orcom_bin/FileStream.h"
#include "../orcom_bin/Params.h"


class DnarchFileBase
{
public:
	virtual ~DnarchFileBase() {}

protected:
	struct DnarchFileHeader
	{
		static const uint32 ReservedBytes = 3;
		static const uint32 HeaderSize = 8 + 4 + ReservedBytes + 9;

		uint64 footerOffset;
		uint32 footerSize;

		uchar reserved[ReservedBytes];

		MinimizerParameters minParams;

		DnarchFileHeader()
		{
			STATIC_ASSERT(sizeof(DnarchFileHeader) == HeaderSize);
		}
	};

	struct DnarchFileFooter
	{
		std::vector<uint64> blockSizes;
	};

	DnarchFileHeader fileHeader;
	DnarchFileFooter fileFooter;
};



class DnarchFileWriter : public DnarchFileBase
{
public:
	DnarchFileWriter();
	~DnarchFileWriter();

	void StartCompress(const std::string& fileName_, const MinimizerParameters& minParams_, const CompressorParams& compParams_);
	void WriteNextBin(const CompressedDnaBlock* bin_);
	void FinishCompress();

	const std::vector<uint64> GetStreamSizes() const
	{
		return streamSizes;
	}

protected:
	FileStreamWriter* metaStream;
	FileStreamWriter* dataStream;

	CompressorParams compParams;

	std::vector<uint64> streamSizes;	// for DEBUG purposes

	void WriteFileHeader();
	void WriteFileFooter();
};


class DnarchFileReader : public DnarchFileBase
{
public:
	DnarchFileReader();
	~DnarchFileReader();

	void StartDecompress(const std::string& fileName_, MinimizerParameters& minParams_);
	bool ReadNextBin(CompressedDnaBlock *bin_);
	void FinishDecompress();

protected:
	FileStreamReader* metaStream;
	FileStreamReader* dataStream;

	uint64 blockIdx;

	void ReadFileHeader();
	void ReadFileFooter();
};

#endif // DNARCHFILE_H
