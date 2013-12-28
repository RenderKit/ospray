#pragma once

#include "../common/managed.h"
#include "../common/ospcommon.h"

namespace ospray {
  struct Model;

  struct Geometry : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Geometry"; }
    virtual void finalize(Model *model) {}
  };

};
