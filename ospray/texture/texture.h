/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once 

#include "ospray/common/managed.h"

namespace ospray {
  /*! \brief implements the basic abstraction for anything that is a 'texture'.

   */
  struct Texture : public ManagedObject 
  {
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::Texture"; }

    ///*! \brief creates an abstract texture class of a given type
    // *
    // * The respective texture type must be a registered texture type
    // * in either ospray proper or an already loaded module. For texture
    // * types specified in special modules, make sure to call
    // * ospLoadModule first.
    // */
    //static Texture *createTexture(const char *identifier);
  };

  /* I ended up following the model of registering a given texture type
   * with the thought that we may want different internal implementations
   * for floating point textures vs integer textures and different channels per pixel
   */
//#define OSP_REGISTER_TEXTURE(InternalClassName,external_name)   \
//  extern "C" Texture *ospray_create_texture__##external_name()  \
//  {                                                             \
//    return new InternalClassName;                               \
//  }
}
