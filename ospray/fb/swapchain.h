#pragma once

#include "framebuffer.h"
#include <vector>

namespace ospray {
  struct SwapChain;
  struct FrameBuffer;

  /*! typedef for a 'factory' function that returns frame buffers of a given type */
  typedef FrameBuffer *(*FrameBufferFactory)(const vec2i &fbSize);

  /*! class that implements a 'swap chain' of frame buffers of a given size */
  struct SwapChain {
    SwapChain(const size_t numFBs, 
              const vec2i &fbSize, 
              FrameBufferFactory fbFactory);

    std::vector<Ref<FrameBuffer> > swapChain;
    const vec2i               fbSize;
    FrameBufferFactory        fbFactory;
  };
}
