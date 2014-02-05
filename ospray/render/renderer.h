#pragma once

#include "../common/model.h"
#include "../fb/framebuffer.h"
//#include "loadbalancer.h"

/*! \file render.h Defines the base renderer class */

namespace ospray {
  
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

    static Renderer *createRenderer(const char *identifier);
  };

  /*! \brief abstract for all renderers that use tiled load balancing 

    it's up to the load balancer to call this renderer on a given set
    of tiles; there is no guarantee which tiles this tile renderer's
    'renderTile' function is being called, nor even how many such
    calls are made for any given frame (for dynamic load balancing the
    set of tiles may change from frame to frame
   */
  struct TileRenderer : public Renderer {
    /*! abstraction for a render task. 

      note that this is _not_ derived from embree::taskscheduler::task
      because we cannot know how many tiles there will actually
      be. the actual 'task' (in that sense) will be handled by the
      load balancer. The main purpose of this rendertask is thus _not_
      to do thread-parallel task scheduling, but rather to be able to
      do certain per-frame proprocessing (e.g., parameter parsing) in
      'createRenderTask()' and to store the resulting data in a form
      that is valid 'til the end of the frame */
    struct RenderJob : public embree::RefCount {
      /*! \brief render given tile */
      virtual void renderTile(Tile &tile) = 0;
    };
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::TileRenderer"; }

    /*! \brief create a task for rendering */
    virtual RenderJob *createRenderJob(FrameBuffer *fb) = 0;
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

