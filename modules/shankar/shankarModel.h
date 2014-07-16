/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ospray/common/model.h"

namespace ospray {
  namespace shankar {
    struct Point {
      vec3f pos;
      vec3f nor;
      float rad;
    };

    struct Surface {
      std::vector<Point> point;

      void load(const std::string &fn);
      /*! for when we are using the adamson/alexa model */
      void computeNormalsAndRadii();

      box3f getBounds() const;
    };

    struct Model {
      std::vector<Surface *> surface;
      box3f getBounds() const;
    };
  }
}

