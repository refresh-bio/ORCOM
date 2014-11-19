/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#include "../orcom_bin/Globals.h"

#include <iostream>
#include <string.h>
#include <cstdio>

#include "main.h"

#include "../orcom_bin/BinModule.h"

#include "../orcom_pack/DnarchModule.h"
#include "../orcom_pack/Params.h"

#include "../orcom_bin/Utils.h"
#include "../orcom_bin/Thread.h"

uint32 InputArguments::AvailableCoresNumber = mt::thread::hardware_concurrency();
uint32 InputArguments::DefaultThreadNumber = MIN(8, InputArguments::AvailableCoresNumber);
std::string InputArguments::DefaultTmpBinFile = "__tmp_orcom";


int main(int argc_, const char* argv_[])
{
	if (argc_ < 1 + 3 || (argv_[1][0] != 'e' && argv_[1][0] != 'd'))
	{
		usage();
		return -1;
	}

	InputArguments args;
	if (!parse_arguments(argc_, argv_, args))
		return -1;

	if (args.mode == InputArguments::EncodeMode)
		return fastq2dnarch(args);
	return dnarch2dna(args);
}


void usage()
{
	std::cerr << "Overlapping Reads COmpression with Minimizers\n";
	std::cerr << "orcom_pack - DNA bins compression tool\n";
	std::cerr << "Version: " << APP_VERSION << '\n';
	std::cerr << "Authors: Sebastian Deorowicz, Szymon Grabowski and Lukasz Roguski\n\n";

	std::cerr << "usage:\torcom <e|d> [options]\n\n";

	std::cerr << "[e]ncoding options:\n";
	std::cerr << "\t-i<file>\t: input FASTQ file" << '\n';
	std::cerr << "\t-f\"<f1> .. <fn>\": input FASTQ file list" << '\n';
	std::cerr << "\t-g\t\t: input compressed in .gz format\n";
	std::cerr << "\t-o<f>\t\t: archive output files prefix" << "\n";
	std::cerr << "\t-p<n>\t\t: signature length, default: " << MinimizerParameters::DefaultSignatureLen << '\n';
	std::cerr << "\t-s<n>\t\t: skip-zone length, default: " << MinimizerParameters::DefaultskipZoneLen << "\n";
	std::cerr << "\t-e<n>\t\t: encode threshold value, default: 0 (0 - auto)\n";
	std::cerr << "\t-m<n>\t\t: mismatch cost, default: " << CompressorParams::DefaultMismatchCost << '\n';
	std::cerr << "\t-c<n>\t\t: insert cost, default: " << CompressorParams::DefaultInsertCost << "\n";
	std::cerr << "\t-b<n>\t\t: FASTQ input buffer size (in MB), default: " << (BinModuleConfig::DefaultFastqBlockSize >> 20) << '\n';
	std::cerr << "\t-T<n>\t\t: temporary bins file, default: " << InputArguments::DefaultTmpBinFile << '\n';
#if (DEV_TWEAK_MODE)
	std::cerr << "\t-l<n>\t\t: signature suffix len, default: " << MinimizerParameters::DefaultSignatureSuffixLen << '\n';
	std::cerr << "\t-m<n>\t\t: mimimum block bin size, default: " << CategorizerParameters::DefaultMinimumPartialBinSize << '\n';
	std::cerr << "\t-r\t\t: disable reverse-compliment strategy\n";
	std::cerr << "\t-n<n>\t\t: max encode value, default: " << CompressorParams::DefaultMaxCostValue << '\n';
	std::cerr << "\t-x<n>\t\t: minimum bin size to filter, default: " << CompressorParams::DefaultMinimumBinSize << '\n';
#endif
	std::cerr << '\n';

	std::cerr << "[d]ecoding options:\n";
	std::cerr << "\t-i<file>\t: input archive files prefix" << '\n';
	std::cerr << "\t-o<f>\t\t: output DNA stream file" << "\n";
	std::cerr << '\n';

	std::cerr << "common options:\n";
	std::cerr << "\t-t<n>\t\t: threads count, default: " << InputArguments::DefaultThreadNumber << '\n';
}


int fastq2dnarch(const InputArguments& args_)
{
	try
	{
		BinModule module;

		module.SetModuleConfig(args_.binConfig);
		module.Fastq2Bin(args_.inputFiles, args_.tmpBinFile, args_.threadsNum, args_.compressedInput);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error clustering records: " << e.what() << std::endl;
		return -1;
	}

	try
	{
		DnarchModule module;
		module.Bin2Dnarch(args_.tmpBinFile, args_.outputFile, args_.compParams, args_.threadsNum);

		std::remove(args_.tmpBinFile.c_str());
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error compressing records: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}


int dnarch2dna(const InputArguments& args_)
{
	try
	{
		DnarchModule module;
		module.Dnarch2Dna(args_.inputFiles[0], args_.outputFile, args_.threadsNum);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error decompressing records: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}


bool parse_arguments(int argc_, const char* argv_[], InputArguments& outArgs_)
{
	outArgs_.mode = (argv_[1][0] == 'e') ? InputArguments::EncodeMode : InputArguments::DecodeMode;

	// parse params
	//
	for (int i = 2; i < argc_; ++i)
	{
		const char* param = argv_[i];
		if (param[0] != '-')
			continue;

		int pval = -1;
		int len = strlen(param);
		if (len > 2 && len < 10)
		{
			pval = to_num((const uchar*)param + 2, len - 2);
			ASSERT(pval >= 0);
		}
		const char* str = param + 2;
		const uint32 slen = len - 2;

		switch (param[1])
		{
			// general options
			//
			case 'i':	outArgs_.inputFiles.push_back(std::string(str, str + slen));	break;
			case 'f':
			{
				int beg = 2;
				for (int i = 2; i < len-1; ++i)
				{
					if (param[i] == ' ' || param[i] == '\n')
					{
						outArgs_.inputFiles.push_back(std::string(param + beg, i - beg));
						beg = i + 1;
					}
				}
				outArgs_.inputFiles.push_back(std::string(param + beg, len - beg));

				break;
			}
			case 'o':	outArgs_.outputFile.assign(str, str + slen);					break;
			case 'g':	outArgs_.compressedInput = true;								break;

			// clustering options
			//
			case 'p':	outArgs_.binConfig.minimizer.signatureLen = pval;				break;
			case 's':	outArgs_.binConfig.minimizer.skipZoneLen = pval;				break;

			// encoding options
			//
			case 'e':	outArgs_.compParams.encodeThresholdValue = pval;				break;
			case 'c':	outArgs_.compParams.insertCost = pval;							break;
			case 'm':	outArgs_.compParams.mismatchCost = pval;						break;

			// processing options
			//
			case 'b':	outArgs_.binConfig.fastqBlockSize = (uint64)pval << 20;			break;
			case 't':	outArgs_.threadsNum = pval;										break;
			case 'T':	outArgs_.tmpBinFile.assign(str, str + slen);					break;

#if (DEV_TWEAK_MODE)
			case 'l':	outArgs_.binConfig.minimizer.signatureSuffixLen = pval;			break;
			case 'r':	outArgs_.binConfig.minimizer.tryReverseCompliment = false;		break;
			case 'm':	outArgs_.binConfig.catParams.minBlockBinSize = pval;			break;
			case 'n':	outArgs_.compParams.maxCostValue = pval;						break;
			case 'x':	outArgs_.compParams.minBinSize = pval;							break;
#endif
		}
	}

	// check params
	//
	if (outArgs_.inputFiles.size() == 0)
	{
		std::cerr << "Error: no input file(s) specified\n";
		return false;
	}

	if (outArgs_.outputFile.length() == 0)
	{
		std::cerr << "Error: no output file specified\n";
		return false;
	}

	if (outArgs_.threadsNum == 0 || outArgs_.threadsNum > 64)
	{
		std::cerr << "Error: invalid number of threads specified\n";
		return false;
	}

	return true;
}
