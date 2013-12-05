#include "ospray.h"
// embree:
#include "embree2/rtcore.h"
//embree/include/embree.h"

// C-lib
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>

namespace ospray {
  extern bool logAPI;

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
      } else if (parm == "--osp:log-api") {
        logAPI = true;
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

