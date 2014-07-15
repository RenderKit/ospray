/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
