#pragma once

#include "volume.h"

namespace ospray {

  /*! \brief "Bricked32": Implements a volume with bricking, and at
      most 32bits of addressing

      \note Knowing that this has only 32 bits of addressing allows
      for using vector gather instructions in ISPC mode
  */
  template<typename T>
  struct BrickedVolumeSampler : public StructuredVolumeSampler<T> {
    BrickedVolumeSampler();
    //BrickedVolume(const vec3i &size, const T *internalData=NULL);

    // virtual void setRegion(const vec3i &where,const vec3i &size,const uint8 *data);
    // virtual void setRegion(const vec3i &where,const vec3i &size,const float *data);
    void setRegion(const vec3i &where,const vec3i &size,const T *data);
    virtual std::string toString() const { return "BrickedVolumeSampler<T>"; };

    //! tri-linear interpolation at given sample location; for single, individual sample
    virtual float lerpf(const vec3fa &samplePos);

    const vec3i      size;
#ifdef LOW_LEVEL_KERNELS
    vec3fa  clampSize;
    int voxelOffset[8];
#endif
  };

  
}
