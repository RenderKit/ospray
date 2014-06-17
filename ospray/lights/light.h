#pragma once

#include "ospray/common/managed.h"

namespace ospray {

  struct Light : public ManagedObject {
    static Light *createLight(const char *type);
  };

#define OSP_REGISTER_LIGHT(InternalClassName, external_name)        \
  extern "C" ospray::Light *ospray_create_light__##external_name()  \
  {                                                                 \
    return new InternalClassName;                                   \
  }

}
