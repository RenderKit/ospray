#pragma once

#include "renderer.h"

namespace ospray {
  /*! test renderer that renders a simple test image using ispc */
  struct RayCastRenderer : public Renderer {
    virtual std::string toString() const { return "ospray::RayCastRenderer"; }
    virtual void renderFrame(FrameBuffer *fb);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
