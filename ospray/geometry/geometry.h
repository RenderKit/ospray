#pragma once

#include "../common/managed.h"
#include "../common/ospcommon.h"

namespace ospray {
  struct Model;

  struct Geometry : public ManagedObject
  {
    virtual std::string toString() const { return "ospray::Geometry"; }
    virtual void finalize(Model *model) {}

    static Geometry *createGeometry(const char *identifier);
  };

  /*! \brief registers a internal ospray::<ClassName> geometry under
      the externally accessible name "external_name" 
      
      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      geometry. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this geometry.
  */
#define OSP_REGISTER_GEOMETRY(InternalClassName,external_name)      \
  extern "C" ospray::Geometry *ospray_create_geometry__##external_name() \
  {                                                                 \
    return new InternalClassName;                                   \
  }                                                                 \
  
}
