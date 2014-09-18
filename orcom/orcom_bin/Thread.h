#ifndef THREAD_H
#define THREAD_H

#include "Globals.h"

#ifdef USE_BOOST_THREAD
#include <boost/thread.hpp>
namespace mt = boost;
#else
#include <thread>
#include <mutex>
#include <condition_variable>
namespace mt = std;
#endif

#endif // THREAD_H
