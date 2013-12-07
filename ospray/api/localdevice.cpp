#include "localdevice.h"
#include "../fb/swapchain.h"

namespace ospray {
  namespace api {
    OSPFrameBuffer 
    LocalDevice::frameBufferCreate(const vec2i &size, 
                                   const OSPFrameBufferMode mode,
                                   const size_t swapChainDepth)
    {
      FrameBufferFactory fbFactory = NULL;
      switch(mode) {
      case OSP_RGBA_I8:
        fbFactory = createLocalFB_RGBA_I8;
        break;
      default:
        AssertError("frame buffer mode not yet supported");
      }
      SwapChain *sc = new SwapChain(swapChainDepth,size,fbFactory);
      Assert(sc != NULL);
      return (OSPFrameBuffer)sc;
    }
    
  }
}

