#ifndef DNARCHMODULE_H
#define DNARCHMODULE_H

#include "../orcom_bin/Globals.h"

#include <string>

#include "Params.h"


class DnarchModule
{
public:
	void Bin2Dnarch(const std::string& inBinFile_, const std::string& outDnarchFile_, const CompressorParams& params_, uint32 threadsNum_ = 1);
	void Dnarch2Dna(const std::string& inDnarchFile_, const std::string& outDnaFile_, uint32 threadsNum_ = 1);
};

#endif // DNARCHMODULE_H
