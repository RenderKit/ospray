/*! \file multihit_kernel.ispc \brief Implements the (ISPC-side) multi-hit kernel */

// ospray
#include "multihit_kernel.ih"

//! declares the externally implmented hand-coded multi-hit kernel for 8-wide vectorization
extern "C" void handcoded_multiHitKernel8(void *uniform valid, 
                                          void *uniform retValue,
                                          RTCScene scene, 
                                          void *uniform ray,
                                          void *uniform hitArray,
                                          int   uniform hitArraySize);

/*! \brief find intersections along the given ray, and return the
  (up to) 'hitArraySize' closest hits along that ray, in sorted
  order. The ray itself may or may not be modified. */
size_t multiHitKernel(RTCScene                 scene,
                      varying Ray             &ray,
                      varying MHTKHit *uniform hitArray,
                      const uniform uint32     hitArraySize)
{
  varying bool mask = __mask;
  unmasked {
    varying int imask = mask ? -1 : 0;
  }

  if (sizeof(varying float) == 32) {
    varying int numFound;
    handcoded_multiHitKernel8(&imask,&numFound,scene,&ray,hitArray,hitArraySize);
    return numFound;
  } else print("multihit-kernel not yet implemented for anything other than 8-wide AVX");
  // else if (sizeof(varying float) == 32)
  //   ispcIntersect8(&imask,scene,&ray);
  // else if (sizeof(varying float) == 64)
  //   ispcIntersect16(&imask,scene,&ray);
}

