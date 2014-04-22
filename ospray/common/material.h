#pragma once 

#include "managed.h"

namespace ospray {
  /*! \brief implements the basic abstraction for anything that is a 'material'.

    Note that different renderers will probably define different materials, so the same "logical" material (such a as a "diffuse gray" material) may look differently */
  struct Material : public ManagedObject 
  {
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::Material"; }

    //! \brief commit the material's parameters
    virtual void commit() {}

    /*! \brief creates an abstract material class of given type 

      The respective material type must be a registered material type
      in either ospray proper or any already loaded module. For
      material types specified in special modules, make sure to call
      ospLoadModule first. */
    static Material *createMaterial(const char *identifier);
  };
}
