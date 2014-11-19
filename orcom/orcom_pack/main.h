/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_MAIN
#define H_MAIN

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
		return args;
	}
};


void usage();
int bin2dnarch(const InputArguments& args_);
int dnarch2dna(const InputArguments& args_);
bool parse_arguments(int argc_, const char* argv_[], InputArguments& outArgs_);

int main(int argc_, const char* argv_[]);


#endif // H_MAIN
