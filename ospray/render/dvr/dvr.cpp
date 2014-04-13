#include "dvr.h"
#include "../camera/perspectivecamera.h"
#include "../volume/volume.h"

namespace ospray {
  extern "C" void ispc__DVRRenderer_renderFrame(void *camera, void *scene, void *fb);

  void DVRRenderer::renderFrame(FrameBuffer *fb)
  {
    Assert(fb && "null frame buffer handle");
    void *_fb = fb->getIE();
    Assert(_fb && "invalid host-side-only frame buffer");

    /* for now, let's get the volume data from the renderer
       parameter. at some point we probably want to have voluems
       embedded in models - possibly with transformation attached to
       them - but for now let's do this as simple as possible */
    Volume *volume = (Volume *)getParam("volume",NULL);
    Assert(volume && "null volume handle in 'dvr' renderer "
           "(did you forget to assign a 'volume' parameter to the renderer?)");
    void *_volume = (void*)volume->getIE();
    Assert(_volume && "invalid volume without a ISPC-side volume "
           "(did you forget to finalize/'commit' the volume?)");

    Camera *camera = (Camera *)getParam("camera",NULL);
    Assert(camera && "null camera handle in 'dvr' renderer "
           "(did you forget to assign a 'camera' parameter to the renderer?)");
    void *_camera = (void*)camera->getIE();
    Assert(_camera && "invalid camera without a ISPC-side camera "
           "(did you forget to finalize/'commit' the camera?)");
    
    ispc__DVRRenderer_renderFrame(_camera,_volume,_fb);
  }

  OSP_REGISTER_RENDERER(DVRRenderer,dvr);
};

