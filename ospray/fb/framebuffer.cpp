#include "framebuffer.h"

namespace ospray {

  extern "C" void *ispc__createLocalFB_RGBA_F32(uint32,uint32,void*,void*);
  extern "C" void *ispc__createLocalFB_RGBA_I8(uint32,uint32,void*,void*);

  /*! the LocalFrameBuffer constructor uses this method to create an
    ISPC-side frame buffer instance that corresponds to the c-side
    one */
  template<class PixelType>
  void *createLocalFrameBufferISPC(const vec2i &size, 
                                   void *classPtr, void *pixelArray)
  { 
    PING;
    Assert(false && "creating ispc frame buffer not yet implemented for this pixel type"); 
    return NULL; 
  }

  struct foo { float x, y, z; };

  /*! the LocalFrameBuffer constructor uses this method to create an
    ISPC-side frame buffer instance that corresponds to the c-side
    one */
  template<>
  void *createLocalFrameBufferISPC<vec4f>(const vec2i &size, 
                                          void *classPtr, void *pixelArray)
  { 
    void *fb = ispc__createLocalFB_RGBA_F32(size.x,size.y,classPtr,pixelArray); 
    return fb;
  }
  
  /*! the LocalFrameBuffer constructor uses this method to create an
    ISPC-side frame buffer instance that corresponds to the c-side
    one */
  template<>
  void *createLocalFrameBufferISPC<uint32>(const vec2i &size, 
                                   void *classPtr, void *pixelArray)
  { 
    return ispc__createLocalFB_RGBA_I8(size.x,size.y,classPtr,pixelArray); 
  }

  /*! the 'factory' method that the swapchain can use to create new
      instances of this frame buffer type */
  FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size, void *pixel)
  {
    Assert(size.x > 0);
    Assert(size.y > 0);
    return new LocalFrameBuffer<uint32>(size,pixel);
  }

  template<typename PixelType>
  LocalFrameBuffer<PixelType>::LocalFrameBuffer(const vec2i &size,
                                                void *pixelMem) 
    : FrameBuffer(size), isMapped(false)
  { 
    Assert(size.x > 0);
    Assert(size.y > 0);
    pixel = pixelMem ? (PixelType*)pixelMem : new PixelType[size.x*size.y]; 
    pixelArrayIsMine = pixelMem ? 0 : 1;
    Assert(pixel);
    ispcEquivalent = createLocalFrameBufferISPC<PixelType>(size,(void*)this,(void*)pixel);
    Assert(ispcEquivalent && "frame buffer class of this type not yet implemented in ISPC");
  }
  
  template<typename PixelType>
  LocalFrameBuffer<PixelType>::~LocalFrameBuffer() 
  {
    if (pixelArrayIsMine)
      delete[] pixel; 
  }

  template<typename PixelType>
  const void *LocalFrameBuffer<PixelType>::map()
  {
    if (renderTask != NULL) {
      //      printf("waiting fr %lx / %lx\n",this,renderTask.ptr);
      renderTask->done.sync();
      renderTask = NULL;
    }
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
