#pragma once

#include "ospray/render/renderer.h"

namespace ospray {
  /*! \brief Implements a simple "Direct Volume (ray casting) Renderer" (DVR) */
  struct DVRRenderer : public Renderer {
    virtual std::string toString() const { return "ospray::DVRRenderer"; }
    virtual void renderFrame(FrameBuffer *fb);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
