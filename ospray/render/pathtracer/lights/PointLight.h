#pragma once

#include "api/parms.h"
#include "pointlight_ispc.h"

namespace embree
{
  struct PointLight
  {
    static void* create(const Parms& parms)
    {
      const Vector3f P = parms.getVector3f("P",zero);
      const Color I = parms.getColor("I",zero);
      return ispc::PointLight__new((ispc::vec3f&)P,(ispc::vec3f&)I);
    }
  };
}

