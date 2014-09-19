/*
  This file is a part of ORCOM software distributed under GNU GPL 2 licence.
  Homepage:	http://sun.aei.polsl.pl/orcom
  Github:	http://github.com/lrog/orcom

  Authors: Sebastian Deorowicz, Szymon Grabowski and Lucas Roguski
*/

#ifndef H_GLOBALS
#define H_GLOBALS

#ifndef NDEBUG
#	define DEBUG 1
#endif


// Visual Studio warning supression
//
#if defined (_WIN32)
#	define _CRT_SECURE_NO_WARNINGS
#	pragma warning(disable : 4996) // D_SCL_SECURE
#	pragma warning(disable : 4244) // conversion uint64 to uint32
#	pragma warning(disable : 4267)
#	pragma warning(disable : 4800) // conversion byte to bool
#endif


// assertions
//
#if defined(DEBUG) || defined(_DEBUG)
#	include "assert.h"
#	define ASSERT(x) assert(x)
#else
#	define ASSERT(x) (void)(x)
#endif
#define COMPILE_TIME_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(!!(COND))*2-1]
#define COMPILE_TIME_ASSERT1(X, L) COMPILE_TIME_ASSERT(X,static_assertion_at_line_##L)
#define COMPILE_TIME_ASSERT2(X, L) COMPILE_TIME_ASSERT1(X,L)
#define STATIC_ASSERT(X)    COMPILE_TIME_ASSERT2(X,__LINE__)


// global macros
//
#define BIT(x)							(1 << (x))
#define MIN(x,y)						((x) <= (y) ? (x) : (y))
#define MAX(x,y)						((x) >= (y) ? (x) : (y))
#define ABS(x)							((x) >=  0  ? (x) : -(x))
#define TFREE(ptr)						{ if (ptr != NULL) delete ptr; ptr = NULL; }


// app version control
//
#define APP_VERSION						"1.0a"


// experimental features
//
#define EXP_USE_RC_ADV					0

#define DEV_TWEAK_MODE					0
#define DEV_PRINT_STATS					0


// basic types
//
#include <stdint.h>
typedef int8_t				int8;
typedef uint8_t				uchar, byte, uint8;
typedef int16_t				int16;
typedef uint16_t			uint16;
typedef int32_t				int32;
typedef uint32_t			uint32;
typedef int64_t				int64;
typedef uint64_t			uint64;



// common interfaces
//
class IOperator
{
public:
	virtual ~IOperator() {}

	void operator() ()
	{
		Run();
	}

	virtual void Run() = 0;
};

struct ICoder
{
	virtual ~ICoder() {}

	virtual void Start() = 0;
	virtual void End() = 0;
};


// TODO: split declarations depending on the project -- bin/pack
//
class DnaRecord;
class BitMemoryReader;
class BitMemoryWriter;
class IFastqStreamReader;
class IFastqStreamWriter;
class BinFileReader;
class BinFileWriter;
class BinaryBinBlock;
class DnaBinBlock;
class DataChunk;
class DnaBin;
class Buffer;
class MinimizerParameters;
class CategorizerParameters;
class BinModuleConfig;


#endif // H_GLOBALS
