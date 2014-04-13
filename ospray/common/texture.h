#pragma once 

#include "managed.h"

namespace ospray {
  /*! \brief implements the basic abstraction for anything that is a 'texture'.

   */
  struct Texture : public ManagedObject 
  {
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::Texture"; }
  };
}
