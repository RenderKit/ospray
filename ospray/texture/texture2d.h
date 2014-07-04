#pragma once

#include "texture.h"
#include "../include/ospray/ospray.h"

namespace ospray {

  struct Texture2D : public Texture {
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::Texture2D"; }

    /*! \brief creates a Texture2D object with the given parameter */
    static Texture2D *createTexture(int width, int height, OSPDataType type, void *data, int flags);

    int width;
    int height;
    OSPDataType type;
    void *data;
  };

}
