#pragma once

// ospray
#include "../common/ospcommon.h"
#include "../common/managed.h"
#include "tile.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  extern "C" void ispc__setTile(void *_fb, void *_tile);

  struct RenderFrameEvent : public RefCount {
    TaskScheduler::EventSync done;
  };

  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    /*! event that lets us register that 'somebody' is still busy
        rendering this frame buffer */
    
    // TaskScheduler::EventSync *doneRendering;
    Ref<RenderFrameEvent> renderTask;

    const vec2i size;
    FrameBuffer(const vec2i &size) : size(size) {
      Assert(size.x > 0 && size.y > 0);
    };
    virtual const void *map() = 0;
    virtual void unmap(const void *mappedMem) = 0;

    // virtual void setTile(Tile &tile) = 0;

    /*! call ISPC-side to write the given tile into the frame buffer.
      though this function does different things for different frame
      buffer layouts it is not virtual itself, since this virtual
      behavior is already achieved via the function pointer on the
      ispc-side equivalent of this class */
    inline void setTile(Tile &tile)
    { ispc__setTile(getIE(),&tile); }
  };


  /*! local frame buffer - frame buffer that exists on local machine */
  template<typename PixelType>
  struct LocalFrameBuffer : public FrameBuffer {
    PixelType *pixel;
    bool       isMapped;

    LocalFrameBuffer(const vec2i &size);
    virtual ~LocalFrameBuffer();

    virtual const void *map();
    virtual void unmap(const void *mappedMem);
    // virtual void setTile(Tile &tile);

  };

  FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size);
}
