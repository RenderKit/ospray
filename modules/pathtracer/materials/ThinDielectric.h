#pragma once

#include "api/parms.h"
#include "thindielectric_ispc.h"

namespace embree
{
  struct ThinDielectric
  {
    static void* create(const Parms& parms)
    {
      const Color transmission = parms.getColor("transmission",one);
      const float eta          = parms.getFloat("eta",1.4f);
      const float thickness    = parms.getFloat("thickness",0.1f);
      return ispc::ThinDielectric__new((ispc::vec3f&)transmission,eta,thickness);
    }
  };
}

