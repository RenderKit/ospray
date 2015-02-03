#pragma once

#include "api/parms.h"
#include "distantlight_ispc.h"

namespace ospray {
  namespace pt {
    
    struct DistantLight
    {
      static void* create(const Parms& parms)
      {
        const Vector3f D = parms.getVector3f("D");
        const Color L = parms.getColor("L");
        const float halfAngle = parms.getFloat("halfAngle");
        return ispc::DistantLight__new((ispc::vec3f&)D,(ispc::vec3f&)L,halfAngle);
      }
    };

  } // ::ospray::pt
} // ::ospray

