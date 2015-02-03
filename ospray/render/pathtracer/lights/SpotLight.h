#pragma once

#include "api/parms.h"
#include "spotlight_ispc.h"

namespace ospray {
  namespace pt {
    
    struct SpotLight
    {
      static void* create(const Parms& parms)
      {
        const Vector3f P = parms.getVector3f("P");
        const Vector3f D = parms.getVector3f("D");
        const Color I = parms.getColor("I");
        const float angleMin = parms.getFloat("angleMin");
        const float angleMax = parms.getFloat("angleMax");
        return ispc::SpotLight__new((ispc::vec3f&)P,(ispc::vec3f&)D,(ispc::vec3f&)I,angleMin,angleMax);
      }

    } // ::ospray::pt
  } // ::ospray

