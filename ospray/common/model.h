#pragma once

// ospray stuff
#include "../geometry/geometry.h"
// stl stuff
#include <vector>
// embree stuff
#include "rtcore.h"
#include "rtcore_scene.h"
#include "rtcore_geometry.h"

namespace ospray {

  struct Model : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Model"; }
    virtual void finalize();

    std::vector<Ref<Geometry> > geometry;

    //! \brief the embree scene handle for this geometry
    RTCScene eScene; 
  };

};
