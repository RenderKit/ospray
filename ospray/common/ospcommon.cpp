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
  uint logLevel = 0;
  
  void doAssertion(const char *file, int line, const char *expr, const char *expl) {
    if (expl)
      fprintf(stderr,"%s:%u: Assertion failed: \"%s\":\nAdditional Info: %s\n", file, line, expr, expl);
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
    bool debugMode = false;
    for (int i=1;i<ac;) {
      std::string parm = av[i];
      if (parm == "--osp:debug") {
        debugMode = true;
        removeArgs(ac,av,i,1);
      } else {
        ++i;
      }
    }

    // initialize embree:
    std::stringstream embreeConfig;
    if (debugMode)
      embreeConfig << " threads=1";
    rtcInit(embreeConfig.str().c_str());
    assert(rtcGetError() == RTC_NO_ERROR);

    // if (debugMode) {
    //   embree::rtcStartThreads(0);
    // } else {
    //   // how many !?
    //   embree::rtcStartThreads(0);
    // }
  }

  void removeArgs(int &ac, char **&av, int where, int howMany)
  {
    for (int i=where+howMany;i<ac;i++)
      av[i-howMany] = av[i];
    ac -= howMany;
  }

}

