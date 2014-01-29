#pragma once

#include "../common/model.h"
#include "../fb/framebuffer.h"
#include "loadbalancer.h"

/*! \file render.h Defines the base renderer class */

namespace ospray {
  
  /*! abstract base class for all ospray renderers. */
  struct Renderer : public ManagedObject {
    virtual std::string toString() const { return "ospray::Renderer"; }
    virtual void renderFrame(FrameBuffer *fb) = 0;

    static Renderer *createRenderer(const char *identifier);
  };

  struct TileRenderer : public Renderer {
    virtual void renderTile(Tile &tile) = 0;
    virtual void renderFrame(FrameBuffer *fb);
  };

  /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name" 
      
      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
  */
#define OSP_REGISTER_RENDERER(InternalClassName,external_name)      \
  extern "C" Renderer *ospray_create_renderer__##external_name()    \
  {                                                                 \
    return new InternalClassName;                                   \
  }                                                                 \
  
}

