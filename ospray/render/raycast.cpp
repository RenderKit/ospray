// ospray
#include "raycast.h"
#include "../camera/perspectivecamera.h"
// embree
#include "common/sys/sync/atomic.h"

namespace ospray {
  extern "C" void ispc__RayCastRenderer_renderFrame(void *camera, void *scene, void *fb);
  extern "C" void ispc__RayCastRenderer_renderTile(void *tile, void *camera, void *scene);

  // void RayCastRenderer::RenderTask::run(size_t threadIndex, size_t threadCount, size_t taskIndex, size_t taskCount, TaskScheduler::Event* event)
  // {
  //   // PING;
  //   // PING;
  //   // PING;
  //   // PRINT(threadIndex);
  //   // PRINT(threadCount);
  //   // PRINT(taskIndex);
  //   // PRINT(taskCount);
  // }

  void RayCastRenderer::renderTile(Tile &tile)
  {
    // Assert(fb && "null frame buffer handle");
    // void *_fb = fb->getIE();
    // Assert2(_fb,"invalid host-side-only frame buffer");

    Model *world = (Model *)getParam("world",NULL);
    Assert2(world,"null world handle (did you forget to assign a 'world' parameter to the ray_cast renderer?)");
    void *_scene = (void*)world->eScene;
    Assert2(_scene,"invalid model without an embree scene (did you forget to finalize/'commit' the model?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert2(camera,"null camera handle (did you forget to assign a 'camera' parameter to the ray_cast renderer?)");
    void *_camera = (void*)camera->getIE();
    Assert2(_camera,"invalid model without a ISPC-side camera "
           "(did you forget to finalize/'commit' the camera?)");

    // PING; PRINT(tile.region);

    ispc__RayCastRenderer_renderTile(&tile,_camera,_scene);
  }
#if 0
  void RayCastRenderer::renderFrame(FrameBuffer *fb)
  {
    Assert(fb && "null frame buffer handle");
    void *_fb = fb->getIE();
    Assert2(_fb,"invalid host-side-only frame buffer");

    Model *world = (Model *)getParam("world",NULL);
    Assert2(world,"null world handle (did you forget to assign a 'world' parameter to the ray_cast renderer?)");
    void *_scene = (void*)world->eScene;
    Assert2(_scene,"invalid model without an embree scene (did you forget to finalize/'commit' the model?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert2(camera,"null camera handle (did you forget to assign a 'camera' parameter to the ray_cast renderer?)");
    void *_camera = (void*)camera->getIE();
    Assert2(_camera,"invalid model without a ISPC-side camera "
           "(did you forget to finalize/'commit' the camera?)");

// #if 1
//     PING;
//     RenderTask *renderTask = new RenderTask;
//     // renderTask.run = (runFunction)renderTask._run;
//     PING;
//     TaskScheduler::addTask(-1, TaskScheduler::GLOBAL_BACK, renderTask); //&task_renderTiles);
//     PING;
// #else
    ispc__RayCastRenderer_renderFrame(_camera,_scene,_fb);
// #endif
  }
#endif

  OSP_REGISTER_RENDERER(RayCastRenderer,ray_cast);
  OSP_REGISTER_RENDERER(RayCastRenderer,raycast);
};

