#include "swapchain.h"

namespace ospray {
  SwapChain::SwapChain(const size_t numFBs, 
                       const vec2i &fbSize, 
                       FrameBufferFactory fbFactory)
    : fbSize(fbSize), fbFactory(fbFactory)
  {
    Assert(numFBs > 0 && numFBs < 4);
    Assert(fbFactory != NULL);

    swapChain.resize(numFBs);
    for (int i=0;i<numFBs;i++) {
      swapChain[i] = (*fbFactory)(fbSize);
      Assert(swapChain[i] != NULL);
    }
  }
  
}
