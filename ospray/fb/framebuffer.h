#pragma once

#include "../common/ospcommon.h"
#include "../common/managed.h"

namespace ospray {
  /*! abstract frame buffer class */
  struct FrameBuffer : public ManagedObject {
    const vec2i size;
    FrameBuffer(const vec2i &size) : size(size) {
      Assert(size.x > 0 && size.y > 0);
    };
    virtual const void *map() = 0;
    virtual void unmap(const void *mappedMem) = 0;

    /*! returns an ISPC-side representation of this class */
    virtual void *inISPC() = 0;
  };

  /*! local frame buffer - frame buffer that exists on local machine */
  template<typename PixelType>
  struct LocalFrameBuffer : public FrameBuffer {
    PixelType *pixel;
    void *ispcPtr; /*!< pointer to ISPC-side frame buffer */
    bool  isMapped;

    LocalFrameBuffer(const vec2i &size);
    virtual ~LocalFrameBuffer();

    virtual const void *map();
    virtual void unmap(const void *mappedMem);

    /*! returns an ISPC-side representation of this class */
    virtual void *inISPC();
  };

  FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size);
}
