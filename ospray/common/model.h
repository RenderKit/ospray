#include "managed.h"

namespace ospray {

  struct Model : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Model"; }
  };

};
