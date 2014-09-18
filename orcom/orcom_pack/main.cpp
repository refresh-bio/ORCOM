#include "../orcom_bin/Globals.h"

#include <iostream>
#include <string.h>

#include "main.h"
#include "DnarchModule.h"
#include "Params.h"

#include "../orcom_bin/Utils.h"
#include "../orcom_bin/Thread.h"

uint32 InputArguments::AvailableCoresNumber = mt::thread::hardware_concurrency();
uint32 InputArguments::DefaultThreadNumber = MIN(8, InputArguments::AvailableCoresNumber);


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
		return bin2dnarch(args);
	return dnarch2dna(args);
}

void usage()
{
	std::cerr << "Overlapping Reads COmpression with Minimizers\n";
	std::cerr << "orcom_pack - DNA bins compression tool\n";
	std::cerr << "Version: " << APP_VERSION << '\n';
	std::cerr << "Authors: Sebastian Deorowicz, Szymon Grabowski and Lukasz Roguski\n\n";

	std::cerr << "usage:\n\torcom_pack <e|d> [options] -i<input_file> -o<output_file>\n";
	std::cerr << "options:\n";

	std::cerr << "\t-i<file>\t: orcom_bin generated input files prefix\n";
	std::cerr << "\t-o<file>\t: output files prefix\n";

	std::cerr << "\t-e<n>\t\t: encode threshold value, default: 0 (0 - auto)\n";
	std::cerr << "\t-m<n>\t\t: mismatch cost, default: " << CompressorParams::DefaultMismatchCost << '\n';
	std::cerr << "\t-s<n>\t\t: insert cost, default: " << CompressorParams::DefaultInsertCost << '\n';
	std::cerr << "\t-t<n>\t\t: threads count, default: " << InputArguments::DefaultThreadNumber << '\n';

#if (DEV_TWEAK_MODE)
	std::cerr << "\t-n<n>\t\t: max encode value, default: " << CompressorParams::DefaultMaxCostValue << '\n';
	std::cerr << "\t-f<n>\t\t: minimum bin size to filter, default: " << CompressorParams::DefaultMinimumBinSize << '\n';
#endif
}


int bin2dnarch(const InputArguments& args_)
{
	try
	{
		DnarchModule module;

		module.Bin2Dnarch(args_.inputFile, args_.outputFile, args_.params, args_.threadsNum);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}

int dnarch2dna(const InputArguments& args_)
{
	try
	{
		DnarchModule module;
		module.Dnarch2Dna(args_.inputFile, args_.outputFile, args_.threadsNum);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
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
			case 'i':	outArgs_.inputFile.assign(str, str + slen);		break;
			case 'o':	outArgs_.outputFile.assign(str, str + slen);	break;

			case 'e':	outArgs_.params.encodeThresholdValue = pval;	break;
			case 's':	outArgs_.params.insertCost = pval;				break;
			case 'm':	outArgs_.params.mismatchCost = pval;			break;
			case 't':	outArgs_.threadsNum = pval;						break;
#if (DEV_TWEAK_MODE)
			case 'n':	outArgs_.params.maxCostValue = pval;			break;
			case 'f':	outArgs_.params.minBinSize = pval;				break;
#endif
		}
	}

	// check params
	//
	if (outArgs_.inputFile.size() == 0)
	{
		std::cerr << "Error: no input file specified\n";
		return false;
	}

	if (outArgs_.outputFile.length() == 0)
	{
		std::cerr << "Error: no output file specified\n";
		return false;
	}

	return true;
}
