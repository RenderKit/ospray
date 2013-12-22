#pragma once

#include "renderer.h"

namespace ospray {
  /*! test renderer that renders a simple test image using ispc */
  struct TestRenderer : public Renderer {
    virtual std::string toString() const { return "ospray::TestRenderer"; }
    virtual void renderFrame(FrameBuffer *fb,
                             Model *world);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
