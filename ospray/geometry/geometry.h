#pragma once

#include "../common/managed.h"

namespace ospray {

  struct Geometry : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Geometry"; }
  };

};
