/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// #pragma once

// #include "framebuffer.h"
// #include <vector>

// namespace ospray {
//   struct SwapChain;
//   struct FrameBuffer;

//   /*! typedef for a 'factory' function that returns frame buffers of a given type */
//   typedef FrameBuffer *(*FrameBufferFactory)(const vec2i &fbSize, void *mem);

//   /*! class that implements a 'swap chain' of frame buffers of a given size */
//   struct SwapChain : public ManagedObject {
//     SwapChain(const size_t numFBs, 
//               const vec2i &fbSize, 
//               FrameBufferFactory fbFactory);
//     const void *map();
//     void unmap(const void *mapped);

//     /*! return framebuffer on frontend side (the one we can map on the host) */
//     FrameBuffer *getFrontBuffer() 
//     { return swapChain[frontBufferPos].ptr; }
//     /*! return framebuffer on backend side (the one the next renderer will write to) */
//     FrameBuffer *getBackBuffer()  
//     { return swapChain[(frontBufferPos+1) % swapChain.size()].ptr; }
//     void advance() { frontBufferPos = (frontBufferPos+1)%swapChain.size(); }
//     virtual std::string toString() const { return "ospray::SwapChain"; }

//     std::vector<Ref<FrameBuffer> > swapChain;
//     std::vector<int64>             lastRendered;
//     const vec2i                    fbSize;
//     FrameBufferFactory             fbFactory;
//     int                            frontBufferPos;
//   };
// }
