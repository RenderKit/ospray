// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "ospray/common/parallel_for.h"
#include "BlockBrickedVolume_ispc.h"
// std
#include <cassert>

namespace ospray {

  BlockBrickedVolume::~BlockBrickedVolume()
  {
    if (ispcEquivalent) {
      ispc::BlockBrickedVolume_freeVolume(ispcEquivalent);
    }
  }

  std::string BlockBrickedVolume::toString() const
  {
    return("ospray::BlockBrickedVolume<" + voxelType + ">");
  }

  void BlockBrickedVolume::commit()
  {
    // The ISPC volume container should already exist. We (currently)
    // require 'dimensions' etc to be set first, followed by call(s)
    // to 'setRegion', and only a final commit at the
    // end. 'dimensions' etc may/will _not_ be committed before
    // setregion.
    exitOnCondition(ispcEquivalent == nullptr,
                    "the volume data must be set via ospSetRegion() "
                    "prior to commit for this volume type");

    // StructuredVolume commit actions.
    StructuredVolume::commit();
  }

   int BlockBrickedVolume::setRegion(// points to the first voxel to be copies.
                                    // The voxels at 'source' MUST have
                                    // dimensions 'regionSize', must be
                                    // organized in 3D-array order, and must
                                    // have the same voxel type as the volume.
                                    const void *source,
                                    // coordinates of the lower,
                                    // left, front corner of the target
                                    // region.
                                    const vec3i &regionCoords,
                                    // size of the region that we're writing to
                                    // MUST be the same as the dimensions of
                                    // source[][][]
                                    const vec3i &regionSize,
                                    /*! size of the region that the block of data
                                      occupies*/
                                    const vec3i &region
                                    )
  {
    // Create the equivalent ISPC volume container and allocate memory for voxel
    // data.
    if (ispcEquivalent == nullptr) createEquivalentISPC();

    /*! \todo check if we still need this 'computevoxelrange' - in
        theory we need this only if the app is allowed to query these
        values, and they're not being set in sharedstructuredvolume,
        either, so should we actually set them at all!? */
    // Compute the voxel value range for unsigned byte voxels if none was
    // previously specified.
    Assert2(source,"nullptr source in BlockBrickedVolume::setRegion()");

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    if (findParam("voxelRange") == nullptr) {
      // Compute the voxel value range for float voxels if none was
      // previously specified.
      const size_t numVoxelsInRegion
        = (size_t)regionSize.x *
        + (size_t)regionSize.y *
        + (size_t)regionSize.z;
      if (voxelType == "uchar")
        computeVoxelRange((unsigned char *)source, numVoxelsInRegion);
      else if (voxelType == "float")
        computeVoxelRange((float *)source, numVoxelsInRegion);
      else if (voxelType == "double")
        computeVoxelRange((double *) source, numVoxelsInRegion);
      else {
        throw std::runtime_error("invalid voxelType in "
                                 "BlockBrickedVolume::setRegion()");
      }
    }
#endif

    // Copy voxel data into the volume.
    const int NTASKS = regionSize.y * regionSize.z;
    parallel_for(NTASKS, [&](int taskIndex){
      ispc::BlockBrickedVolume_setRegion(ispcEquivalent,
                                         source,
                                         (const ispc::vec3i &) regionCoords,
                                         (const ispc::vec3i &) regionSize,
                                         (const ispc::vec3i &) region,
                                         taskIndex);
    });

    return true;
  }

  void BlockBrickedVolume::createEquivalentISPC()
  {
    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");
    exitOnCondition(getVoxelType() == OSP_UNKNOWN,
                    "unrecognized voxel type (must be set before "
                    "calling ospSetRegion())");

    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0,
                    "invalid volume dimensions (must be set before "
                    "calling ospSetRegion())");

    // Create an ISPC BlockBrickedVolume object and assign type-specific
    // function pointers.
    ispcEquivalent = ispc::BlockBrickedVolume_createInstance(this,
                                         (int)getVoxelType(),
                                         (const ispc::vec3i &)this->dimensions);
  }

#ifdef EXP_NEW_BB_VOLUME_KERNELS
  /*! in new bb kernel mode we'll be using the code in
      GhostBlockBrickedVolume.* */
#else
  // A volume type with 64-bit addressing and multi-level bricked storage order.
  OSP_REGISTER_VOLUME(BlockBrickedVolume, block_bricked_volume);
#endif
} // ::ospray

