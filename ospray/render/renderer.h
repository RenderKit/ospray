#pragma once

/*! \file renderer.h Defines the base renderer class */

#include "../common/model.h"
#include "../fb/framebuffer.h"

namespace ospray {

  struct Material;

  /*! \brief abstract base class for all ospray renderers. 

    the base renderer only knows about 'rendering a frame'; most
    actual renderers will be derived from a tile renderer, but this
    abstraction level also allows for frame compositing or even
    projection/splatting based approaches
   */
  struct Renderer : public ManagedObject {
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Renderer"; }
    /*! \brief produce one frame, and put it into given frame
        buffer */
    virtual void renderFrame(FrameBuffer *fb) = 0;

    /*! \brief create a material of given type */
    virtual Material *createMaterial(const std::string &type) { return NULL; }

    static Renderer *createRenderer(const char *identifier);
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

