#pragma once

// ospray stuff
#include "../geometry/geometry.h"
// stl stuff
#include <vector>
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"

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
