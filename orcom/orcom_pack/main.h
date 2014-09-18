#ifndef MAIN_H
#define MAIN_H

#include "../orcom_bin/Globals.h"

#include <string>

#include "Params.h"

struct InputArguments
{
	enum ModeEnum
	{
		EncodeMode,
		DecodeMode
	};

	static uint32 AvailableCoresNumber;
	static uint32 DefaultThreadNumber;

	ModeEnum mode;

	std::string inputFile;
	std::string outputFile;

	CompressorParams params;
	uint32 threadsNum;

	InputArguments()
		:	threadsNum(DefaultThreadNumber)
	{}

	static InputArguments Default()
	{
		InputArguments args;
		args.params = CompressorParams::Default();
		return args;
	}
};

void usage();
int bin2dnarch(const InputArguments& args_);
int dnarch2dna(const InputArguments& args_);
bool parse_arguments(int argc_, const char* argv_[], InputArguments& outArgs_);

int main(int argc_, const char* argv_[]);


#endif // MAIN_H
