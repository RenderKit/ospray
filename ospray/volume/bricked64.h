#pragma once

#include "volume.h"
#include "bricked64.ih" // including the *ispc* header file here -
                        // shouldn't actually do that, and have the
                        // constants somewhere else ... */

namespace ospray {

  /*! \brief Volume that is both page-bricked and cache-bricked. */
  template<typename T>
  struct PagedBrickedVolume : public StructuredVolume<T> {
    struct CacheBrick {
      T cell[CACHE_BRICK_RES][CACHE_BRICK_RES][CACHE_BRICK_RES];
    };
    struct PageBrick {
      CacheBrick brick
      [PAGE_BRICK_RES/CACHE_BRICK_RES]
      [PAGE_BRICK_RES/CACHE_BRICK_RES]
      [PAGE_BRICK_RES/CACHE_BRICK_RES];
    };
    PagedBrickedVolume() {}
    
    void setRegion(const vec3i &where,const vec3i &size,const T *data);
    virtual std::string toString() const { return "PagedBrickedVolume<T>"; };
    virtual ispc::_Volume *createIE();
    
    //! tri-linear interpolation at given sample location; for single, individual sample
    virtual float lerpf(const vec3fa &samplePos);
    //! gradient at given sample location
    virtual vec3f gradf(const vec3fa &samplePos);

    void allocate();

    vec3ui numPages;
  };
  
}
