/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_DNABLOCKDATA
#define H_DNABLOCKDATA

#include "Globals.h"
#include "Collections.h"


struct DnaBinBlock
{
	DnaBinCollection stdBins;
	DnaBin nBin;

	DnaBinBlock()
	{}

	void Clear()
	{
		stdBins.Clear();
		nBin.Clear();
	}
};


#endif // H_DNABLOCKDATA
