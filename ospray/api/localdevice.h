#include "device.h"

/*! \file localdevice.h Implements the "local" device for local rendering */

namespace ospray {
  namespace api {
    struct LocalDevice : public Device {

      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size, 
                                               const OSPFrameBufferMode mode,
                                               const size_t swapChainDepth);

      /*! map frame buffer */
      virtual const void *frameBufferMap(OSPFrameBuffer fb);

      /*! unmap previously mapped frame buffer */
      virtual void frameBufferUnmap(const void *mapped,
                                    OSPFrameBuffer fb);
    };
  }
}


