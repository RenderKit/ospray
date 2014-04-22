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

  // extern "C" void *ispc_createModel(Model *cppModel, 
  //                                   int32 numGeometries,
  //                                   RTCScene *embreeHandle);
  // extern "C" void ispc_setModelGeometry(void *model, int32 gID, void *geometry);

  void Model::finalize()
  {
    if (logLevel > 2) {
      cout << "=======================================================" << endl;
      cout << "Finalizing model, has " << geometry.size() << " geometries" << endl;
    }

    this->ispcEquivalent = ispc::Model_create(this,geometry.size());
    embreeSceneHandle = (RTCScene)ispc::Model_getEmbreeSceneHandle(getIE());

    // for now, only implement triangular geometry...
    for (int i=0;i<geometry.size();i++) {
      geometry[i]->finalize(this);
      ispc::Model_setGeometry(getIE(),i,geometry[i]->getIE());
    }
    
    rtcCommit(embreeSceneHandle);
  }
}
