/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "framebuffer.h"
#include "localfb_ispc.h"

namespace ospray {

  FrameBuffer::FrameBuffer(const vec2i &size,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer)
    : size(size),
      colorBufferFormat(colorBufferFormat),
      hasDepthBuffer(hasDepthBuffer),
      hasAccumBuffer(hasAccumBuffer),
      accumID(-1)
  {
    Assert(size.x > 0 && size.y > 0);
  };

  void LocalFrameBuffer::clear(const uint32 fbChannelFlags)
  {
    if (fbChannelFlags & OSP_FB_ACCUM) {
      ispc::LocalFrameBuffer_clearAccum(getIE());
      accumID = 0;
    }
  }

  LocalFrameBuffer::LocalFrameBuffer(const vec2i &size,
                                     ColorBufferFormat colorBufferFormat,
                                     bool hasDepthBuffer,
                                     bool hasAccumBuffer, 
                                     void *colorBufferToUse)
    : FrameBuffer(size, colorBufferFormat, hasDepthBuffer, hasAccumBuffer)
  { 
    Assert(size.x > 0);
    Assert(size.y > 0);
    if (colorBufferToUse)
      colorBuffer = colorBufferToUse;
    else {
      switch(colorBufferFormat) {
      case OSP_RGBA_NONE:
        colorBuffer = NULL;
        break;
      case OSP_RGBA_F32:
        colorBuffer = new vec4f[size.x*size.y];
        break;
      case OSP_RGBA_I8:
        colorBuffer = new uint32[size.x*size.y];
        break;
      }
    }

    if (hasDepthBuffer)
      depthBuffer = new float[size.x*size.y];
    else
      depthBuffer = NULL;
    
    if (hasAccumBuffer)
      accumBuffer = new vec4f[size.x*size.y];
    else
      accumBuffer = NULL;
    ispcEquivalent = ispc::LocalFrameBuffer_create(this,size.x,size.y,
                                                   colorBufferFormat,
                                                   colorBuffer,
                                                   depthBuffer,
                                                   accumBuffer);
  }
  
  LocalFrameBuffer::~LocalFrameBuffer() 
  {
    if (depthBuffer) delete[] depthBuffer;
    if (colorBuffer) delete[] colorBuffer;
    if (accumBuffer) delete[] accumBuffer;
  }

  void FrameBuffer::waitForRenderTaskToBeReady() 
  {
    // if (frameIsReadyEvent) {
    //   frameIsReadyEvent->sync();
    // }
  }

  const void *LocalFrameBuffer::mapDepthBuffer()
  {
    waitForRenderTaskToBeReady();
    this->refInc();
    return (const void *)depthBuffer;
  }
  
  const void *LocalFrameBuffer::mapColorBuffer()
  {
    waitForRenderTaskToBeReady();
    this->refInc();
    return (const void *)colorBuffer;
  }
  
  void LocalFrameBuffer::unmap(const void *mappedMem)
  {
    Assert(mappedMem == colorBuffer || mappedMem == depthBuffer );
    this->refDec();
  }
}
// #include "framebuffer.h"
// // ispc exports
// #include "framebuffer_ispc.h"

// namespace ospray {

//   extern "C" void *ispc__createLocalFB_RGBA_F32(uint32,uint32,void*,void*);
//   extern "C" void *ispc__createLocalFB_RGBA_I8(uint32,uint32,void*,void*);

//   void FrameBuffer::waitForRenderTaskToBeReady() 
//   {
//     frameIsReadyEvent.sync();
//   }

//   /*! the LocalFrameBuffer constructor uses this method to create an
//     ISPC-side frame buffer instance that corresponds to the c-side
//     one */
//   template<class PixelType>
//   void *createLocalFrameBufferISPC(const vec2i &size, 
//                                    void *classPtr, void *pixelArray)
//   { 
//     PING;
//     Assert(false && "creating ispc frame buffer not yet implemented for this pixel type"); 
//     return NULL; 
//   }

//   struct foo { float x, y, z; };

//   /*! the LocalFrameBuffer constructor uses this method to create an
//     ISPC-side frame buffer instance that corresponds to the c-side
//     one */
//   template<>
//   void *createLocalFrameBufferISPC<vec4f>(const vec2i &size, 
//                                           void *classPtr, void *pixelArray)
//   { 
//     void *fb = ispc__createLocalFB_RGBA_F32(size.x,size.y,classPtr,pixelArray); 
//     return fb;
//   }
  
//   /*! the LocalFrameBuffer constructor uses this method to create an
//     ISPC-side frame buffer instance that corresponds to the c-side
//     one */
//   template<>
//   void *createLocalFrameBufferISPC<uint32>(const vec2i &size, 
//                                    void *classPtr, void *pixelArray)
//   { 
//     return ispc__createLocalFB_RGBA_I8(size.x,size.y,classPtr,pixelArray); 
//   }

//   // /*! the 'factory' method that the swapchain can use to create new
//   //     instances of this frame buffer type */
//   // FrameBuffer *createLocalFB_RGBA_I8(const vec2i &size, void *pixel)
//   // {
//   //   Assert(size.x > 0);
//   //   Assert(size.y > 0);
//   //   return new LocalFrameBuffer<uint32>(size,pixel);
//   // }

//   template<typename PixelType>
//   LocalFrameBuffer<PixelType>::LocalFrameBuffer(const vec2i &size,
//                                                 void *pixelMem) 
//     : FrameBuffer(size), isMapped(false)
//   { 
//     Assert(size.x > 0);
//     Assert(size.y > 0);
//     pixel = pixelMem ? (PixelType*)pixelMem : new PixelType[size.x*size.y]; 
//     pixelArrayIsMine = pixelMem ? 0 : 1;
//     Assert(pixel);
//     ispcEquivalent = createLocalFrameBufferISPC<PixelType>(size,(void*)this,(void*)pixel);
//     Assert(ispcEquivalent && "frame buffer class of this type not yet implemented in ISPC");
//   }
  
//   template<typename PixelType>
//   LocalFrameBuffer<PixelType>::~LocalFrameBuffer() 
//   {
//     if (pixelArrayIsMine)
//       delete[] pixel; 
//   }

//   template<typename PixelType>
//   const void *LocalFrameBuffer<PixelType>::map()
//   {
//     if (renderTask != NULL) {
//       //      printf("waiting fr %lx / %lx\n",this,renderTask.ptr);
//       renderTask->done.sync();
//       renderTask = NULL;
//     }
//     Assert(!this->isMapped);
//     this->refInc();
//     this->isMapped = true;
//     return (const void *)this->pixel;
//   }
  
//   template<typename PixelType>
//   void LocalFrameBuffer<PixelType>::unmap(const void *mappedMem)
//   {
//     Assert(mappedMem == this->pixel);
//     Assert(this->isMapped);
//     this->isMapped = false;
//     this->refDec();
//   }

//   template class LocalFrameBuffer<uint32>;
// }
