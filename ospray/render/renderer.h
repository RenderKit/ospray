#pragma once

#include "../common/model.h"
#include "../fb/framebuffer.h"

/*! \file render.h Defines the base renderer class */

namespace ospray {
  
  /*! abstract base class for all ospray renderers. */
  struct Renderer : public ManagedObject {
    virtual std::string toString() const { return "ospray::Renderer"; }
    virtual void renderFrame(FrameBuffer *fb,
                             Model *world) = 0;
  };
}

