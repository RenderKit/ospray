#pragma once

#include "../common/managed.h"

namespace ospray {
  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    const vec2i size;
    FrameBuffer(const vec2i &size) : size(size) {
      Assert(size.x > 0 && size.y > 0);
    };
  };

  /*! local frame buffer - frame buffer exists on local machine */
  template<typename PixelType>
  struct LocalFrameBuffer : public FrameBuffer {
    PixelType *pixel;
    void *ispcPtr; /*!< pointer to ISPC-side frame buffer */

    LocalFrameBuffer(const vec2i &size) 
      : FrameBuffer(size) 
    { pixel = new PixelType[size.x*size.y]; }
  };

  FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size);
}
