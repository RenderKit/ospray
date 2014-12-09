#pragma once

#include "api/parms.h"
#include "trianglelight_ispc.h"

namespace ospray {
  namespace pt {
    
    struct TriangleLight
    {
      static void* create(const Parms& parms)
      {
        const Vector3f v0 = parms.getVector3f("v0");
        const Vector3f v1 = parms.getVector3f("v1");
        const Vector3f v2 = parms.getVector3f("v2");
        const Color L = parms.getColor("L");
        return ispc::TriangleLight__new((ispc::vec3f&)v0,(ispc::vec3f&)v1,(ispc::vec3f&)v2,(ispc::vec3f&)L);
      }
    };

  } // ::ospray::pt
} // ::ospray

