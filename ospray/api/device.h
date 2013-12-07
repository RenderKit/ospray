#include "ospray/common/ospcommon.h"
#include "ospray/include/ospray/ospray.h"
/*! \file device.h Defines the abstract base class for OSPRay
    "devices" that implement the OSPRay API */

namespace ospray {
  /*! encapsulates everything related to the implementing public ospray api */
  namespace api {

    /*! abstract base class of all 'devices' that implement the ospray API */
    struct Device {
      /*! singleton that points to currently active device */
      static Device *current;

      virtual ~Device() {};

      virtual OSPFrameBuffer 
      frameBufferCreate(const vec2i &size, 
                        const OSPFrameBufferMode mode,
                        const size_t swapChainDepth) = 0;
    };
  }
}

