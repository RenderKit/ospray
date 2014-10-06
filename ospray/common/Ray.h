/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray stuff
#include "ospray/common/OspCommon.h"

namespace ospray {
  /*! \brief ospray *scalar* ray class 

    This impelment the base ospray ray class; it is 'derived'
    from Embree's ray class, but can have additional members that embree
    does not care about. For convenience we do not actually use the
    embree's RTCRay class itself, and instead use our own ray class that
    uses our vec3fs etc (and possibly adds new members). However, we
    emulate the exact layout of embree's rtcray class, and therefore can
    simply typecast it before calling embree's intersectors.

    Since we do call embree's intersectors on this class the
    layout of the class members HAS to (exactly!) match the layout of
    emrbee's RTCRay class (as defined in
    emrbee/include/embree2/rtcore_ray.isph). We can add additional
    members that embree does not know about, and can can use our own
    names and vector classes where approprite (ie, use our 'vec3f dir'
    vs embrees 'float dx,dy,dz;') but there HAS to be a one-to-one
    correspondence between what we store and what embree expects in its
    'intersect' functions */
  struct Ray {
    /* ray input data */
    vec3fa org;  /*!< ray origin */
    vec3fa dir;  /*!< ray direction */
    float t0;   /*!< start of valid ray interval */
    float t;    /*!< end of valid ray interval, or distance to hit point after 'intersect' */
    float time; //!< Time of this ray for motion blur
    int32 mask; //!< Used to mask out objects during traversal
  
    /* hit data */
    vec3fa Ng;    /*! geometry normal. may or may not be set by geometry intersectors */
  
    float u;     //!< Barycentric u coordinate of hit
    float v;     //!< Barycentric v coordinate of hit
  
    int geomID;  //!< geometry ID
    int primID;  //!< primitive ID
    int instID;  //!< instance ID
    // -------------------------------------------------------
    // END OF EMBREE LAYOUT - this is where we can add our own data
    // -------------------------------------------------------
  };

}
