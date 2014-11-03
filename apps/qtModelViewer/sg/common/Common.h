/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray API
#include "ospray/ospray.h"
// ospray 
#include "ospray/common/OspCommon.h"
// STL

namespace ospray {
  namespace sg {

    typedef unsigned int uint;
    
    /*! base node for every scene graph node */
    struct Node;
    /*! c++ wrapper for an ospray model */
    struct World;
    /*! c++ wrapper for an ospray light source */
    struct Light;
    /*! c++ wrapper for an ospray camera */
    struct Geometry;
    /*! c++ wrapper for an ospray geometry type */
    struct Volume;
    /*! c++ wrapper for an ospray volume type */
    struct Camera;
    /*! c++ wrapper for an ospray renderer (the thing that produces
        pixel colors is actualy close to what PBRT calls in
        'integrator' */
    struct Integrator;

    
  } // ::ospray::sg
} // ::ospray


