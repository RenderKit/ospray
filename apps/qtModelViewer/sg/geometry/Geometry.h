/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    /*! a geometry node - the generic geometry node */
    struct Geometry : public sg::Node {
      Geometry(const std::string &type) : type(type) {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Geometry"; }

      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type; 

      virtual box3f getBounds() = 0;
    };

    
  } // ::ospray::sg
} // ::ospray


