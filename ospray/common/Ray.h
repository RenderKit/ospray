// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

// ospray stuff
#include "common/OSPCommon.h"

namespace ospray {
  /*! \brief ospray *scalar* ray class

    This implement the base ospray ray class; it is 'derived'
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
  struct OSPRAY_SDK_INTERFACE Ray {
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

} // ::ospray
