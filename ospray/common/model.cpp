#include "model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"

namespace ospray {
  using std::cout;
  using std::endl;

  void Model::finalize()
  {
    if (logLevel > 2) {
      cout << "=======================================================" << endl;
      cout << "Finalizing model, has " << geometry.size() << " geometries" << endl;
    }

    eScene = rtcNewScene(
                         //                         RTC_SCENE_COMPACT|
                         RTC_SCENE_STATIC|RTC_SCENE_HIGH_QUALITY,
#if OSPRAY_SPMD_WIDTH==16
                         RTC_INTERSECT1|RTC_INTERSECT16
#elif OSPRAY_SPMD_WIDTH==8
                         RTC_INTERSECT1|RTC_INTERSECT8
#elif OSPRAY_SPMD_WIDTH==4
                         RTC_INTERSECT1|RTC_INTERSECT4
#else
#  error("invalid OSPRAY_SPMD_WIDTH value")
#endif
                         );
    // for now, only implement triangular geometry...
    for (int i=0;i<geometry.size();i++) {
      geometry[i]->finalize(this);
    }
    
    rtcCommit(eScene);
  }
}
