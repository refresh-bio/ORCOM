/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_MAIN
#define H_MAIN

#include "Globals.h"

#include <string>
#include <vector>

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
	BinModuleConfig config;

	bool compressedInput;
	uint32 threadsNum;

	std::vector<std::string> inputFiles;
	std::string outputFile;

	InputArguments()
		:	compressedInput(false)
		,	threadsNum(DefaultThreadNumber)
	{}

	static InputArguments Default()
	{
		InputArguments args;
		return args;
	}
};


void usage();
int fastq2bin(const InputArguments& args_);
int bin2dna(const InputArguments& args_);
bool parse_arguments(int argc_, const char* argv_[], InputArguments& outArgs_);

int main(int argc_, const char* argv_[]);


#endif //H_ MAIN
