#pragma once

// ospray
#include "tile.h"

// ospray
#include "../common/ospcommon.h"
#include "../common/managed.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    /*! app-mappable format of the color buffer. make sure that this
        matches the definition on the ISPC side */
    typedef enum { 
      NONE, /*!< app will never map the color buffer (e.g., for a
               framebuffer attached to a display wall that will likely
               have a different res that the app has...) */
      RGBA_FLOAT32, /*! app will map in RBGA, one float per channel */
      RGBA_UINT8, /*! app will map in RGBA, one uint8 per channel */
    } ColorBufferFormat;

    // the event that triggers when this task is done
    // embree::TaskScheduler::EventSync *frameIsReadyEvent;
    
    const vec2i size;
    FrameBuffer(const vec2i &size,
                ColorBufferFormat colorBufferFormat,
                bool hasDepthBuffer,
                bool hasAccumBuffer);
    virtual const void *mapDepthBuffer() = 0;
    virtual const void *mapColorBuffer() = 0;

    virtual void unmap(const void *mappedMem) = 0;

    /*! make sure the current frame is finished */
    void waitForRenderTaskToBeReady();

    /*! indicates whether the app requested this frame buffer to have
        an accumulation buffer */
    bool hasAccumBuffer;
    /*! indicates whether the app requested this frame buffer to have
        an (application-mappable) depth buffer */
    bool hasDepthBuffer;

    /*! indicates whether the app requested this frame buffer to have
        an (application-mappable) depth buffer */
    ColorBufferFormat colorBufferFormat;

    /*! tracks how many times we have already accumulated into this
        frame buffer. A value of '<0' means that accumulation is
        disabled (in which case the renderer may not access the
        accumulation buffer); in all other cases this value indicates
        how many frames have already been accumulated in this frame
        buffer. Note that it is up to the application to properly
        reset the accumulationID (using ospClearAccum(fb)) if anything
        changes that requires clearing the accumulation buffer. */
    int32 accumID;
  };

  
  /*! local frame buffer - frame buffer that exists on local machine */
  struct LocalFrameBuffer : public FrameBuffer {
    void      *colorBuffer; /*!< format depends on
                               FrameBuffer::colorBufferFormat, may be
                               NULL */
    float     *depthBuffer; /*!< one float per pixel, may be NULL */
    vec4f     *accumBuffer; /*!< one RGBA per pixel, may be NULL */
    
    LocalFrameBuffer(const vec2i &size,
                     ColorBufferFormat colorBufferFormat,
                     bool hasDepthBuffer,
                     bool hasAccumBuffer);
    virtual ~LocalFrameBuffer();
    
    virtual const void *mapColorBuffer();
    virtual const void *mapDepthBuffer();
    virtual void unmap(const void *mappedMem);
  };
}
// #pragma once

// // ospray
// #include "../common/ospcommon.h"
// #include "../common/managed.h"
// #include "tile.h"
// // embree
// #include "common/sys/taskscheduler.h"
// // ispc includes
// #include "framebuffer_ispc.h"

// namespace ospray {
//   using embree::TaskScheduler;

//   extern "C" void ispc__setTile(void *_fb, void *_tile);

//   /*! abstract frame buffer class */
//   struct FrameBuffer : public ManagedObject {
//     // the event that triggers when this task is done
//     embree::TaskScheduler::EventSync frameIsReadyEvent;

//     const vec2i size;
//     FrameBuffer(const vec2i &size) : size(size) {
//       Assert(size.x > 0 && size.y > 0);
//     };
//     virtual const void *map() = 0;
//     virtual void unmap(const void *mappedMem) = 0;

//     // virtual void setTile(Tile &tile) = 0;

//     /*! call ISPC-side to write the given tile into the frame buffer.
//       though this function does different things for different frame
//       buffer layouts it is not virtual itself, since this virtual
//       behavior is already achieved via the function pointer on the
//       ispc-side equivalent of this class */
//     inline void setTile(Tile &tile)
//     { ispc::setTile(getIE(),&tile); }

//     /*! call ISPC-side to write the given tile into the frame buffer.
//       though this function does different things for different frame
//       buffer layouts it is not virtual itself, since this virtual
//       behavior is already achieved via the function pointer on the
//       ispc-side equivalent of this class */
//     inline void accumTile(Tile &tile)
//     { ispc::accumTile(getIE(),&tile); }
//   };


//   /*! local frame buffer - frame buffer that exists on local machine */
//   template<typename PixelType>
//   struct LocalFrameBuffer : public FrameBuffer {
//     PixelType *pixel;
//     bool       isMapped;
//     bool       pixelArrayIsMine;

//     LocalFrameBuffer(const vec2i &size, void *pixelMem = NULL);
//     virtual ~LocalFrameBuffer();

//     virtual const void *map();
//     virtual void unmap(const void *mappedMem);
//     // virtual void setTile(Tile &tile);

//   };

//   // FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size, void *mem);
// }
