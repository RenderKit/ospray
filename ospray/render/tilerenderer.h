#pragma once

/*! \file tilerenderer.h Defines the base class for any tile-based renderer */

#include "renderer.h"

namespace ospray {
  
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

}


