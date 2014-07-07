#undef NDEBUG

// ospray
#include "raycast.h"
#include "ospray/camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"
// ispc exports
#include "raycast_ispc.h"

namespace ospray {

  template<void *(*ISPC_CREATE)(void*)>
  RayCastRenderer<ISPC_CREATE>::RayCastRenderer()
    : model(NULL), camera(NULL) 
  {
    ispcEquivalent = ISPC_CREATE(this);
  }


  template<void *(*ISPC_CREATE)(void*)>
  void RayCastRenderer<ISPC_CREATE>::commit()
  {
    Renderer::commit();

    model = (Model *)getParamObject("world",NULL);
    model = (Model *)getParamObject("model",model);
    camera = (Camera *)getParamObject("camera",NULL);

    ispc::RayCastRenderer_set(getIE(),
                              model?model->getIE():NULL,
                              camera?camera->getIE():NULL);
  }

#if 0
  typedef RayCastRenderer<RC_EYELIGHT> RayCastRenderer_EyeLight;
  typedef RayCastRenderer<RC_PRIMID>   RayCastRenderer_PrimID;
  typedef RayCastRenderer<RC_GEOMID>   RayCastRenderer_GeomID;
  typedef RayCastRenderer<RC_INSTID>   RayCastRenderer_InstID;
  typedef RayCastRenderer<RC_EYELIGHT_PRIMID>   RayCastRenderer_PrimID;
  typedef RayCastRenderer<RC_EYELIGHT_GEOMID>   RayCastRenderer_GeomID;
  typedef RayCastRenderer<RC_EYELIGHT_INSTID>   RayCastRenderer_InstID;
  typedef RayCastRenderer<RC_GNORMAL>  RayCastRenderer_Ng;
  typedef RayCastRenderer<RC_TESTSHADOW>  RayCastRenderer_Shadow;

  OSP_REGISTER_RENDERER(RayCastRenderer_EyeLight,raycast);
  OSP_REGISTER_RENDERER(RayCastRenderer_EyeLight,raycast_eyelight);
  OSP_REGISTER_RENDERER(RayCastRenderer_PrimID,  raycast_eyelight_primID);
  OSP_REGISTER_RENDERER(RayCastRenderer_GeomID,  raycast_eyelight_geomID);
  OSP_REGISTER_RENDERER(RayCastRenderer_InstID,  raycast_eyelight_instID);
  OSP_REGISTER_RENDERER(RayCastRenderer_PrimID,  raycast_primID);
  OSP_REGISTER_RENDERER(RayCastRenderer_GeomID,  raycast_geomID);
  OSP_REGISTER_RENDERER(RayCastRenderer_InstID,  raycast_instID);
  OSP_REGISTER_RENDERER(RayCastRenderer_Ng,      raycast_Ng);
  OSP_REGISTER_RENDERER(RayCastRenderer_Shadow,  raycast_shadow);
#endif

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight> RayCastRenderer_eyeLight;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,raycast);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,eyeLight);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight,raycast_eyelight);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_primID>
  RayCastRenderer_eyeLight_primID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_primID,eyeLight_primID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_primID,primID);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_instID>
  RayCastRenderer_eyeLight_instID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_instID,eyeLight_instID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_instID,instID);

  typedef RayCastRenderer<ispc::RayCastRenderer_create_eyeLight_geomID>
  RayCastRenderer_eyeLight_geomID;
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_geomID,eyeLight_geomID);
  OSP_REGISTER_RENDERER(RayCastRenderer_eyeLight_geomID,geomID);

};

