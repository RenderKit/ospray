#pragma once

// ospray stuff
#include "../geometry/geometry.h"
// stl stuff
#include <vector>

namespace ospray {

  struct Model : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Model"; }
    virtual void finalize();

    std::vector<Ref<Geometry> > geometry;
  };

};
