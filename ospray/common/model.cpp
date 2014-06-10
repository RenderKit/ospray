#include "model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
// ispc side
#include "model_ispc.h"

namespace ospray {
  using std::cout;
  using std::endl;

  void Model::finalize()
  {
    PING;

    if (logLevel > 2) {
      cout << "=======================================================" << endl;
      cout << "Finalizing model, has " << geometry.size() << " geometries" << endl;
    }

    this->ispcEquivalent = ispc::Model_create(this,geometry.size());
    PING;
    embreeSceneHandle = (RTCScene)ispc::Model_getEmbreeSceneHandle(getIE());
    PING;

    // for now, only implement triangular geometry...
    for (int i=0;i<geometry.size();i++) {
      PING;
      PRINT(geometry[i].ptr);
      geometry[i]->finalize(this);
      PING;
      ispc::Model_setGeometry(getIE(),i,geometry[i]->getIE());
      PING;
    }
    
    PING;
    rtcCommit(embreeSceneHandle);
    PING;
  }
}
