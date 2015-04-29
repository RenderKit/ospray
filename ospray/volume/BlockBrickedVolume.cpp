// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

//ospray
#include "ospray/volume/BlockBrickedVolume.h"
#include "BlockBrickedVolume_ispc.h"
// std
#include <cassert>

namespace ospray {

  void BlockBrickedVolume::commit()
  {
    //! The ISPC volume container should already exist.
    exitOnCondition(ispcEquivalent == NULL, "the volume data must be set via ospSetRegion() prior to commit for this volume type");

    //! StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  int BlockBrickedVolume::setRegion(const void *source, const vec3i &index, const vec3i &count)
  {
    //! Create the equivalent ISPC volume container and allocate memory for voxel data.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    //! Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "float" && findParam("voxelRange") == NULL) computeVoxelRange((float *) source, count.x * count.y * count.z);

    //! Compute the voxel value range for unsigned byte voxels if none was previously specified.
    if (voxelType == "uchar" && findParam("voxelRange") == NULL) computeVoxelRange((unsigned char *) source, count.x * count.y * count.z);

    //! Copy voxel data into the volume (the number of voxels must be less than 2**31 - 1 to avoid overflow in voxel indexing in ISPC).
    ispc::BlockBrickedVolume_setRegion(ispcEquivalent, source, (const ispc::vec3i &) index, (const ispc::vec3i &) count);

    //! DO ME: this return value should indicate the success or failure of memory allocation in ISPC and a range check.
    return true;
  }

  void BlockBrickedVolume::createEquivalentISPC() 
  {
    //! Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, "unrecognized voxel type (must be set before calling ospSetRegion())");

    //! Get the volume dimensions.
    vec3i dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(dimensions) <= 0, "invalid volume dimensions (must be set before calling ospSetRegion())");

    //! Create an ISPC BlockBrickedVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::BlockBrickedVolume_createInstance((int)getVoxelType(), (const ispc::vec3i &)dimensions);
  }

} // ::ospray

