/*! \file multihit_kernel.h \brief Defines the public (C-side) interface for
    the multi-hit kernel */

// ospray
#include "common/ospcommon.h"
#include "ospray/common/ray.h"
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



