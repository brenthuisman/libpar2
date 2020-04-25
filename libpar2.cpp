/*
 * par2cmdline() is a function identical to par2's main().
 * It takes the exact same arguments and procudes the exact same outputs,
 * has exactly the same functionality.
 * 
 * The main difference is that after running ./compile.sh, you'll end up with a
 * single file shared library that you can easily use in other projects. My main
 * motivation for this is that I found no way to use the program as a library,
 * and after a long and unpleasant struggle with automake an libtool decided to
 * simply not use the existing makefile, nor bother with the old style of letting
 * the linker sort out includes and symbol availability, but do it (nearly) the
 * SQLite way and simply include all source into a single file.
 * 
 * The code below is almost and exact copy of par2cmdline.cpp, with the
 * most notable change the function name which is properly externed so you end up
 * with a C library you can call from anywhere. The "work" I put in into this
 * source file is figuring out the proper order of includes such that it resolves
 * all symbols correctly, and fix a few things with what appears implicit 'using's
 * and primary type renames.
 * 
 * The /src dir is untouched from upstream, and will remain so in order to make it
 * straightforward to track updates.
 * 
 * Copyright Brent Huisman, par2cmdline authors
 * 
 *******************************************************************************/

// PACKAGE and VERSION are set by configure in par2cmdline, in libpar2 below.
#define PACKAGE "libpar2"
#define VERSION "0.8.1"

#include <string>
using std::string;
#include <ostream>
using std::ostream;
#include <iostream>
using std::endl;
using std::flush;
using std::hex;
#include <iomanip>
using std::setfill;
using std::dec;
using std::setw;

using std::min;

#include <cstring>
#include <cassert>
#include <linux/types.h>
#include "src/libpar2internal.h"
#include "src/libpar2.h" //for u64 like types
#include <stdint.h>
typedef uint8_t            u8;
typedef int8_t             i8;
typedef uint16_t           u16;
typedef int16_t            i16;
typedef uint32_t           u32;
typedef int32_t            i32;
//typedef uint64_t           u64;
typedef int64_t            i64;

#include "src/diskfile.h"
#include "src/md5.h"
#include "src/crc.h"
#include "src/galois.h"
#include "src/reedsolomon.cpp"
#include "src/reedsolomon.h"
#include "src/letype.h"
#include "src/par1fileformat.h"
#include "src/par2fileformat.h"
#include "src/criticalpacket.h"
#include "src/datablock.h"
#include "src/filechecksummer.h"
#include "src/par2creatorsourcefile.h"
#include "src/par2repairersourcefile.h"
#include "src/verificationhashtable.h"
#include "src/verificationpacket.h"
#include "src/recoverypacket.h"
#include "src/mainpacket.h"
#include "src/creatorpacket.h"
#include "src/descriptionpacket.h"
#include "src/par2creator.h"
#include "src/par1repairersourcefile.h"
#include "src/par2repairer.h"
#include "src/par1repairer.h"
#include "src/commandline.h"

#include "src/descriptionpacket.cpp"
#include "src/par2creator.cpp"
#include "src/par1fileformat.cpp"
#include "src/mainpacket.cpp"
#include "src/verificationhashtable.cpp"
#include "src/datablock.cpp"
#include "src/criticalpacket.cpp"
#include "src/par1repairersourcefile.cpp"
#include "src/galois.cpp"
#include "src/par2repairer.cpp"
#include "src/diskfile.cpp"
#include "src/par1repairer.cpp"
#include "src/commandline.cpp"
#include "src/recoverypacket.cpp"
#include "src/verificationpacket.cpp"
#include "src/par2creatorsourcefile.cpp"
#include "src/filechecksummer.cpp"
#include "src/creatorpacket.cpp"
#include "src/libpar2.cpp"
#include "src/par2fileformat.cpp"
#include "src/crc.cpp"
#include "src/md5.cpp"
#include "src/par2repairersourcefile.cpp"

// This is included here, so that cout and cerr are not used elsewhere.
#include <iostream>

#ifdef _MSC_VER
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#endif

#if   defined _WIN32
#define LIB_PRE __declspec(dllexport)
#elif defined __unix__
#define LIB_PRE
#else
#define LIB_PRE __declspec(dllexport)
#endif

extern "C" LIB_PRE int par2cmdline(int argc_, char *argv_[])
{
	int argc = argc_+1;
	const char* argv[argc];
	argv[0]="fake_executable_filename";
	for (int i = 0; i < argc_; i++){
		argv[i+1] = argv_[i];
	}
	
#ifdef _MSC_VER
  // Memory leak checking
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | /*_CRTDBG_CHECK_CRT_DF | */_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

  // check sizeof integers
  static_assert(sizeof(u8) == 1 || sizeof(i8) == 1
		|| sizeof(u16) == 2 || sizeof(i16) == 1
		|| sizeof(u32) == 4 || sizeof(i32) == 1
		|| sizeof(u64) == 8 || sizeof(i64) == 1,
		"Error: the assumed sizes of integers is wrong!");

  // Parse the command line
  CommandLine *commandline = new CommandLine;

  Result result = eInvalidCommandLineArguments;

  if (commandline->Parse(argc, argv))
  {
    // Which operation was selected
    switch (commandline->GetOperation())
    {
      case CommandLine::opCreate:
	// Create recovery data
	result = par2create(std::cout,
			    std::cerr,
			    commandline->GetNoiseLevel(),
			    commandline->GetMemoryLimit(),
			    commandline->GetBasePath(),
#ifdef _OPENMP
			    commandline->GetNumThreads(),
			    commandline->GetFileThreads(),
#endif
			    commandline->GetParFilename(),
			    commandline->GetExtraFiles(),

			    commandline->GetBlockSize(),
			    
			    commandline->GetFirstRecoveryBlock(),
			    commandline->GetRecoveryFileScheme(),
			    commandline->GetRecoveryFileCount(),
			    commandline->GetRecoveryBlockCount()
			    );

        break;
      case CommandLine::opVerify:
      case CommandLine::opRepair:
        {
          // Verify or Repair damaged files
          switch (commandline->GetVersion())
          {
            case CommandLine::verPar1:
	      result = par1repair(std::cout,
				  std::cerr,
				  commandline->GetNoiseLevel(),
				  commandline->GetMemoryLimit(),
#ifdef _OPENMP
				  commandline->GetNumThreads(),
#endif
				  commandline->GetParFilename(),
				  commandline->GetExtraFiles(),
				  commandline->GetOperation() == CommandLine::opRepair,
				  commandline->GetPurgeFiles());
	      
              break;
            case CommandLine::verPar2:
	      result = par2repair(std::cout,
				  std::cerr,
				  commandline->GetNoiseLevel(),
				  commandline->GetMemoryLimit(),
				  commandline->GetBasePath(),
#ifdef _OPENMP
				  commandline->GetNumThreads(),
				  commandline->GetFileThreads(),
#endif
				  commandline->GetParFilename(),
				  commandline->GetExtraFiles(),
				  commandline->GetOperation() == CommandLine::opRepair,
				  commandline->GetPurgeFiles(),
				  commandline->GetSkipData(),
				  commandline->GetSkipLeaway());
              break;
	    default:
              break;
          }
        }
        break;
      case CommandLine::opNone:
        result = eSuccess;
        break;
      default:
        break;
    }
  }

  delete commandline;

  return result;
}
