/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "ospcommon.h"
// embree:
#include "embree2/rtcore.h"
//embree/include/embree.h"

// C-lib
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

namespace ospray {
  /*! logging level - '0' means 'no logging at all', increasing
      numbers mean increasing verbosity of log messages */
  uint32 logLevel = 0;
  bool debugMode = false;

  /*! for debugging. compute a checksum for given area range... */
  void *computeCheckSum(const void *ptr, size_t numBytes)
  {
    long *end = (long *)((char *)ptr + (numBytes - (numBytes%8)));
    long *mem = (long *)ptr;
    long sum = 0;
    long i = 0;

    int nextPing = 1;
    while (mem < end) {
      sum += (i+13) * *mem;
      ++i;
      ++mem;
      
      // if (i==nextPing) { 
      //  std::cout << "checksum after " << (i*8) << " bytes: " << (int*)sum << std::endl;
      //   nextPing += nextPing;
     // }
    }
    return (void *)sum;
  }

  void doAssertion(const char *file, int line, const char *expr, const char *expl) {
    if (expl)
      fprintf(stderr,"%s:%u: Assertion failed: \"%s\":\nAdditional Info: %s\n", 
              file, line, expr, expl);
    else
      fprintf(stderr,"%s:%u: Assertion failed: \"%s\".\n", file, line, expr);
    abort();
  }

  double getSysTime() {
    struct timeval tp; gettimeofday(&tp,NULL); 
    return double(tp.tv_sec) + double(tp.tv_usec)/1E6; 
  }

  void init(int *_ac, const char ***_av)
  {
    int &ac = *_ac;
    char ** &av = *(char ***)_av;
    debugMode = false;
    for (int i=1;i<ac;) {
      std::string parm = av[i];
      if (parm == "--osp:debug") {
        debugMode = true;
        removeArgs(ac,av,i,1);
      } else if (parm == "--osp:loglevel") {
        logLevel = atoi(av[i+1]);
        removeArgs(ac,av,i,2);
      } else {
        ++i;
      }
    }
  }

  void removeArgs(int &ac, char **&av, int where, int howMany)
  {
    for (int i=where+howMany;i<ac;i++)
      av[i-howMany] = av[i];
    ac -= howMany;
  }

}

