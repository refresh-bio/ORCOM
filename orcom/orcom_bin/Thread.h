/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_THREAD
#define H_THREAD

#include "Globals.h"

#ifdef USE_BOOST_THREAD
#	include <boost/thread.hpp>
	namespace mt = boost;
#else
#	include <thread>
#	include <mutex>
#	include <condition_variable>
	namespace mt = std;
#endif

#endif // H_THREAD
