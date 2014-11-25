/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_DNARCHMODULE
#define H_DNARCHMODULE

#include "../orcom_bin/Globals.h"

#include <string>

#include "Params.h"


class DnarchModule
{
public:
	void Bin2Dnarch(const std::string& inBinFile_, const std::string& outDnarchFile_,
					const CompressorParams& params_, uint32 threadsNum_ = 1, bool verboseMode_ = false);
	void Dnarch2Dna(const std::string& inDnarchFile_, const std::string& outDnaFile_, uint32 threadsNum_ = 1);
};


#endif // H_DNARCHMODULE
