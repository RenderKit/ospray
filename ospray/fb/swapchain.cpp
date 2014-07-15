/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// #include "swapchain.h"

// namespace ospray {
//   SwapChain::SwapChain(const size_t numFBs, 
//                        const vec2i &fbSize, 
//                        FrameBufferFactory fbFactory)
//     : fbSize(fbSize), fbFactory(fbFactory), frontBufferPos(0)
//   {
//     Assert(numFBs > 0 && numFBs < 4);
//     Assert(fbFactory != NULL);

//     swapChain.resize(numFBs);
//     lastRendered.resize(numFBs);
//     for (int i=0;i<numFBs;i++) {
//       swapChain[i] = (*fbFactory)(fbSize,NULL);
//       lastRendered[i] = 0;
//       Assert(swapChain[i] != NULL);
//     }
//   }
  
//   const void *SwapChain::map() 
//   {
//     FrameBuffer *fb = getBackBuffer();
//     if (fb->renderTask) {
//       fb->renderTask->done.sync();
//       fb->renderTask = NULL;
//     }
//     return fb->map();
//   }

//   void SwapChain::unmap(const void *mappedMem) 
//   {
//     return getBackBuffer()->unmap(mappedMem);
//   }
// }
