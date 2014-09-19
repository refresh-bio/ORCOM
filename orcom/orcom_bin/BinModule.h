/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_BINMODUL
#define H_BINMODULE

#include "Globals.h"

#include <string>
#include <vector>

#include "Params.h"


class BinModule
{
public:
	void Fastq2Bin(const std::vector<std::string>& inFastqFiles_, const std::string& outBinFile_, uint32 threadNum_ = 1, bool compressedInput_ = false);
	void Bin2Dna(const std::string& inBinFile_, const std::string& outDnaFile_);

	void SetModuleConfig(const BinModuleConfig& config_)
	{
		config = config_;
	}

private:
	BinModuleConfig config;
};


#endif // H_BINMODULE
