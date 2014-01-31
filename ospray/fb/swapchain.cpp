#include "swapchain.h"

namespace ospray {
  SwapChain::SwapChain(const size_t numFBs, 
                       const vec2i &fbSize, 
                       FrameBufferFactory fbFactory)
    : fbSize(fbSize), fbFactory(fbFactory), frontBufferPos(0)
  {
    Assert(numFBs > 0 && numFBs < 4);
    Assert(fbFactory != NULL);

    swapChain.resize(numFBs);
    lastRendered.resize(numFBs);
    for (int i=0;i<numFBs;i++) {
      swapChain[i] = (*fbFactory)(fbSize);
      lastRendered[i] = 0;
      Assert(swapChain[i] != NULL);
    }
  }
  
  const void *SwapChain::map() 
  {
    // getFrontBuffer()->doneRendering.sync();
    return getBackBuffer()->map();
    // return getFrontBuffer()->map();
  }

  void SwapChain::unmap(const void *mappedMem) 
  {
    // return getFrontBuffer()->unmap(mappedMem);
    return getBackBuffer()->unmap(mappedMem);
  }
}
