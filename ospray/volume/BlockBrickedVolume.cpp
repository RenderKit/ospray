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
    // The ISPC volume container should already exist.
    exitOnCondition(ispcEquivalent == NULL, "the volume data must be set via ospSetRegion() prior to commit for this volume type");

    // StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  int BlockBrickedVolume::setRegion(/* points to the first voxel to be copies. The
                                       voxels at 'source' MUST have dimensions
                                       'regionSize', must be organized in 3D-array
                                       order, and must have the same voxel type as the
                                       volume.*/
                                    const void *source, 
                                    /*! coordinates of the lower,
                                      left, front corner of the target
                                      region.*/
                                    const vec3i &regionCoords, 
                                    /*! size of the region that we're writing to; MUST
                                      be the same as the dimensions of source[][][] */
                                    const vec3i &regionSize)
  {
    // Create the equivalent ISPC volume container and allocate memory for voxel data.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    /*! \todo check if we still need this 'computevoxelrange' - in
        theory we need this only if the app is allowed to query these
        values, and they're not being set in sharedstructuredvolume,
        either, so should we actually set them at all!? */
    // Compute the voxel value range for unsigned byte voxels if none was previously specified.
    if (voxelType == "uchar" && findParam("voxelRange") == NULL) 
      computeVoxelRange((unsigned char *) source, size_t(regionSize.x) * regionSize.y * regionSize.z);

    // Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "float" && findParam("voxelRange") == NULL) 
      computeVoxelRange((float *) source, size_t(regionSize.x) * regionSize.y * regionSize.z);

    // Compute the voxel value range for double voxels if none was previously specified.
    if (voxelType == "double" && findParam("voxelRange") == NULL) 
      computeVoxelRange((double *) source, size_t(regionSize.x) * regionSize.y * regionSize.z);

    // Copy voxel data into the volume.
    ispc::BlockBrickedVolume_setRegion(ispcEquivalent, source, 
                                       (const ispc::vec3i &) regionCoords, 
                                       (const ispc::vec3i &) regionSize);
    return true;
  }

  void BlockBrickedVolume::createEquivalentISPC() 
  {
    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, "unrecognized voxel type (must be set before calling ospSetRegion())");

    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions (must be set before calling ospSetRegion())");

    // Create an ISPC BlockBrickedVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::BlockBrickedVolume_createInstance(this,
                                                             (int)getVoxelType(), 
                                                             (const ispc::vec3i &)this->dimensions);
  }

} // ::ospray

