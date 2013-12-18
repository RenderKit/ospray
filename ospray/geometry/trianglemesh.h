#pragma once

#include "geometry.h"

namespace ospray {

  struct TriangleMesh : public Geometry
  {
    virtual std::string toString() const { return "ospray::TriangleMesh"; }
    virtual void finalize();
  };

};
