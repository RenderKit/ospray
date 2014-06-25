#pragma once

#include "ospray/common/managed.h"

namespace ospray {

  //! Base class for Light objects
  struct Light : public ManagedObject {
    //!Create a light of the given type
    static Light *createLight(const char *type);

    //!Copy understood parameters into class members
    virtual void commit(){}

    //!toString is used to aid in printf debugging
    virtual std::string toString() const { return "ospray::Light"; }
  };

#define OSP_REGISTER_LIGHT(InternalClassName, external_name)        \
  extern "C" ospray::Light *ospray_create_light__##external_name()  \
  {                                                                 \
    return new InternalClassName;                                   \
  }

}
