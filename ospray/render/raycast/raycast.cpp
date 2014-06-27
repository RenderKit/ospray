#undef NDEBUG

// ospray
#include "raycast.h"
#include "ospray/camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "raycast_ispc.h"

namespace ospray {

  template<int SHADE_MODE>
  void RayCastRenderer<SHADE_MODE>::commit()
  {
    if (getIE()) ispc::RayCastRenderer_destroy(getIE());

    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    camera = (Camera *)getParamObject("camera",NULL);

    switch(SHADE_MODE) {
    case RC_EYELIGHT:
      PING;
      ispcEquivalent = ispc::RayCastRenderer_create_eyeLight(this->getIE(),model->getIE(),camera->getIE()); 
      break;
    default:
      throw std::runtime_error("shade mode not implemented...");
    };
  }

  typedef RayCastRenderer<RC_EYELIGHT> RayCastRenderer_EyeLight;
  typedef RayCastRenderer<RC_PRIMID>   RayCastRenderer_PrimID;
  typedef RayCastRenderer<RC_GEOMID>   RayCastRenderer_GeomID;
  typedef RayCastRenderer<RC_INSTID>   RayCastRenderer_InstID;
  typedef RayCastRenderer<RC_GNORMAL>  RayCastRenderer_Ng;
  typedef RayCastRenderer<RC_TESTSHADOW>  RayCastRenderer_Shadow;

  OSP_REGISTER_RENDERER(RayCastRenderer_EyeLight,raycast);
  OSP_REGISTER_RENDERER(RayCastRenderer_EyeLight,raycast_eyelight);
  OSP_REGISTER_RENDERER(RayCastRenderer_PrimID,  raycast_primID);
  OSP_REGISTER_RENDERER(RayCastRenderer_GeomID,  raycast_geomID);
  OSP_REGISTER_RENDERER(RayCastRenderer_InstID,  raycast_instID);
  OSP_REGISTER_RENDERER(RayCastRenderer_Ng,      raycast_Ng);
  OSP_REGISTER_RENDERER(RayCastRenderer_Shadow,  raycast_shadow);
};

