#ifndef BINMODULE_H
#define BINMODULE_H

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


#endif // BINMODULE_H
