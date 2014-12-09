// ospray
#include "Device.h"
#include "ospray/common/OSPCommon.h"
// embree
#include "embree2/rtcore.h"

namespace ospray {

  void error_handler(const RTCError code, const char *str);

  namespace api {

    Device *Device::current = NULL;

    Device::Device() {
      rtcSetErrorFunction(error_handler);
    }

  } // ::ospray::api
} // ::ospray
