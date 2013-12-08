#include "framebuffer.h"

namespace ospray {
  
  FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size)
  {
    Assert(size.x > 0);
    Assert(size.y > 0);
    return new LocalFrameBuffer<unsigned int>(size);
  }

  template<typename PixelType>
  LocalFrameBuffer<PixelType>::LocalFrameBuffer(const vec2i &size) 
    : FrameBuffer(size), isMapped(false)
  { 
    Assert(size.x > 0);
    Assert(size.y > 0);
    pixel = new PixelType[size.x*size.y]; 
  }
  
  template<typename PixelType>
  LocalFrameBuffer<PixelType>::~LocalFrameBuffer() 
  {
    delete[] pixel; 
  }

  template<typename PixelType>
  const void *LocalFrameBuffer<PixelType>::map()
  {
    Assert(!this->isMapped);
    this->refInc();
    this->isMapped = true;
    return (const void *)this->pixel;
  }
  
  template<typename PixelType>
  void LocalFrameBuffer<PixelType>::unmap(const void *mappedMem)
  {
    Assert(mappedMem == this->pixel);
    Assert(this->isMapped);
    this->isMapped = false;
    this->refDec();
  }

}
