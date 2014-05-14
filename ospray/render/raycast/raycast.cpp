#undef NDEBUG

// ospray
#include "raycast.h"
#include "ospray/camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  extern "C" void ispc__RayCastRenderer_renderTile(void *tile, void *camera, void *scene, 
                                                   int shadeMode);

  template<int SHADE_MODE>
  void RayCastRenderer<SHADE_MODE>::RenderTask::renderTile(Tile &tile)
  {
    ispc__RayCastRenderer_renderTile(&tile,_camera,_scene,SHADE_MODE);
  }

  template<int SHADE_MODE>
  TileRenderer::RenderJob *RayCastRenderer<SHADE_MODE>::createRenderJob(FrameBuffer *fb)
  {
    RenderTask *frame = new RenderTask;
    frame->world = (Model *)getParamObject("world",NULL);
    frame->world = (Model *)getParamObject("model",frame->world);
    Assert2(frame->world,"null world handle (did you forget to assign a 'world' parameter to the ray_cast renderer?)");
    frame->_scene = (void*)frame->world->embreeSceneHandle;
    Assert2(frame->_scene,"invalid model without an embree scene (did you forget to finalize/'commit' the model?)");

    frame->camera = (Camera *)getParamObject("camera",NULL);
    Assert2(frame->camera,"null camera handle (did you forget to assign a 'camera' parameter to the ray_cast renderer?)");

    frame->_camera = (void*)frame->camera->getIE();
    Assert2(frame->_camera,"invalid model without a ISPC-side camera "
            "(did you forget to finalize/'commit' the camera?)");
    return frame;
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

