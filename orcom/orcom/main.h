/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_MAIN
#define H_MAIN

#include "../orcom_bin/Globals.h"
#include "../orcom_bin/Params.h"
#include "../orcom_pack/Params.h"

#include <string>
#include <vector>


struct InputArguments
{
	enum ModeEnum
	{
		EncodeMode,
		DecodeMode
	};

	static uint32 AvailableCoresNumber;
	static uint32 DefaultThreadNumber;
	static std::string DefaultTmpBinFile;

	ModeEnum mode;
	bool compressedInput;
	uint32 threadsNum;

	std::vector<std::string> inputFiles;
	std::string outputFile;
	std::string tmpBinFile;

	BinModuleConfig binConfig;
	CompressorParams compParams;

	InputArguments()
		:	compressedInput(false)
		,	threadsNum(DefaultThreadNumber)
		,	tmpBinFile(DefaultTmpBinFile)
	{}

	static InputArguments Default()
	{
		InputArguments args;
		return args;
	}
};


void usage();
int fastq2dnarch(const InputArguments& args_);
int dnarch2dna(const InputArguments& args_);
bool parse_arguments(int argc_, const char* argv_[], InputArguments& outArgs_);

int main(int argc_, const char* argv_[]);


#endif // H_MAIN
