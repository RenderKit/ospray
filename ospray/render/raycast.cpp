#include "raycast.h"
#include "../camera/perspectivecamera.h"

namespace ospray {
  extern "C" void ispc__RayCastRenderer_renderFrame(void *camera, void *scene, void *fb);

  void RayCastRenderer::renderFrame(FrameBuffer *fb)
  {
    // if (g_camera == NULL) {
    //   g_camera = new PerspectiveCamera;
    //   g_camera->findParam("pos",true)->set(vec3f(-1,1,-1));
    //   g_camera->findParam("dir",true)->set(vec3f(+1,-1,+1));
    //   PING;
    //   g_camera->commit();
    // }

    Assert(fb && "null frame buffer handle");
    void *_fb = fb->inISPC();
    Assert(_fb && "invalid host-side-only frame buffer");

    Model *world = (Model *)getParam("world",NULL);
    Assert(world && "null world handle "
           "(did you forget to assign a 'world' parameter to the ray_cast renderer?)");
    void *_scene = (void*)world->eScene;
    Assert(_scene && "invalid model without an embree scene "
           "(did you forget to finalize/'commit' the model?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert(camera && "null camera handle "
           "(did you forget to assign a 'camera' parameter to the ray_cast renderer?)");
    void *_camera = (void*)camera->getIE();
    Assert(_camera && "invalid model without a ISPC-side camera "
           "(did you forget to finalize/'commit' the camera?)");
    
    ispc__RayCastRenderer_renderFrame(_camera,_scene,_fb);
  }

  OSP_REGISTER_RENDERER(RayCastRenderer,ray_cast);
};

