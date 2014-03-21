#pragma once

#include "volume.h"

namespace ospray {

  /*! \brief "Naive32": Implements a volume with naive z-order data
      layout, and at most 32bits of addressing

      \note Knowing that this has only 32 bits of addressing allows
      for using vector gather instructions in ISPC mode
  */
  template<typename T>
  struct NaiveVolume : public StructuredVolume<T> {
    NaiveVolume(const vec3i size=vec3i(-1), const T *data=NULL)
      : StructuredVolume<T>(size,data)
    {}
    
    virtual void setRegion(const vec3i &where,const vec3i &size,const T *data);
    virtual std::string toString() const { return "NaiveVolume<T>"; };
    virtual ispc::_Volume *createIE();

    //! tri-linear interpolation at given sample location; for single, individual sample
    virtual float lerpf(const vec3fa &samplePos);
    //! gradient at given sample location
    virtual vec3f gradf(const vec3fa &samplePos) { NOTIMPLEMENTED; };

    void allocate();

#ifdef LOW_LEVEL_KERNELS
    vec3fa  clampSize;
    int voxelOffset[8];
#endif
  };
  
}
