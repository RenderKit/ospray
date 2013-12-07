#include "device.h"

/*! \file localdevice.h Implements the "local" device for local rendering */

namespace ospray {
  namespace api {
    struct LocalDevice : public Device {

      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size, 
                                               const OSPFrameBufferMode mode,
                                               const size_t swapChainDepth);

    };
  }
}


