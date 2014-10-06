/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



// ospray
#include "ospray/common/OspCommon.h"
#include "ospray/common/Ray.h"
// embree
#include "embree2/rtcore.h"
#include "embree2/rtcore_ray.h"
#include "embree2/rtcore_scene.h"
#include "kernels/common/ray.h"

/*! \defgroup mhtk_module_c Multi-hit Traversal Kernel (MHTK) module (C/C++)
  \ingroup mhtk_module
  \brief Scalar (C/C++) side of the Multi-hit Traversal Kernel (MHTK) module 
*/

namespace ospray {
  /*! \addtogroup mhtk_module_c
   * @{ */

  namespace mhtk {

    /*! \addtogroup mhtk_module_c
     * @{ */

    /*! \brief keeps state of one individual hit. multi-hit kernel
        will get array of those 
    */
    struct HitInfo
    {
      float t; //!< distance along the ray
      float u;
      float v;
      int primID;
      int geomID;
      vec3f Ng;
    };

    /*! \brief find intersections along the given ray, and return the
        (up to) 'hitArraySize' closest hits along that ray, in sorted
        order. The ray itself may or may not be modified. */
    size_t multiHitKernel(RTCScene      scene,
                          ospray::Ray  &ray,
                          HitInfo      *hitArray,
                          size_t        hitArraySize);
    /*! @} */
  }

  /*! @} */
}



