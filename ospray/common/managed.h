#pragma once

#include "ospcommon.h"

namespace ospray {

  /*! defines a basic object whose lifetime is managed by ospray */
  struct ManagedObject : public embree::RefCount
  {
    virtual std::string toString() const { return "ManagedObject"; }
    virtual ~ManagedObject() {};
  };

}
