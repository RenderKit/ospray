/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "volume.h"

namespace ospray {

  /*! \brief "Bricked32": Implements a volume with bricking, and at
    most 32bits of addressing

    \note Knowing that this has only 32 bits of addressing allows
    for using vector gather instructions in ISPC mode
  */
  template<typename T>
  struct BrickedVolume : public StructuredVolume<T> {
    BrickedVolume() {}

    void setRegion(const vec3i &where,const vec3i &size,const T *data);
    virtual std::string toString() const { return "BrickedVolume<T>"; };
    virtual ispc::_Volume *createIE();

    //! tri-linear interpolation at given sample location; for single, individual sample
    virtual float lerpf(const vec3fa &samplePos);
    //! gradient at given sample location
    virtual vec3f gradf(const vec3fa &samplePos);

    void allocate();

    vec3i bricks;
#ifdef LOW_LEVEL_KERNELS
    vec3fa  clampSize;
    int voxelOffset[8];
#endif
  };
  
}
