#include "testrenderer.h"

namespace ospray {
  extern "C" void ispc__TestRenderer_renderFrame(void *fb, int frameID);

  void TestRenderer::renderFrame(FrameBuffer *fb,
                                 Model *world)
  {
    ispc__TestRenderer_renderFrame(fb->inISPC(),frameID++);
  }
};
