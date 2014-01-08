#pragma once

#include "renderer.h"

namespace ospray {
  /*! test renderer that renders a simple test image using ispc */
  struct DVRRenderer : public Renderer {
    virtual std::string toString() const { return "ospray::DVRRenderer"; }
    virtual void renderFrame(FrameBuffer *fb);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
