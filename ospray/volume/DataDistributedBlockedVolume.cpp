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

// ospray
#include "ospray/volume/DataDistributedBlockedVolume.h"
// ispc exports:
#include "DataDistributedBlockedVolume_ispc.h"

namespace ospray {

  //! Allocate storage and populate the volume, called through the OSPRay API.
  void DataDistributedBlockedVolume::commit()
  {
    StructuredVolume::commit();
  }
  
  //! Copy voxels into the volume at the given index (non-zero return value indicates success).
  int DataDistributedBlockedVolume::setRegion(/* points to the first voxel to be copies. The
                                                 voxels at 'soruce' MUST have dimensions
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
    
    float f = 0.5f;
    computeVoxelRange(&f,1);

    return 0;
  }

  void DataDistributedBlockedVolume::createEquivalentISPC()
  {
    if (ispcEquivalent != NULL) return;

    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, 
                    "unrecognized voxel type (must be set before calling ospSetRegion())");
    
    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions (must be set before calling ospSetRegion())");

    ddBlocks    = getParam3i("num_dp_blocks",vec3i(4,4,4));
    blockSize   = divRoundUp(dimensions,ddBlocks);
	std::cout << "#osp:dp: using data parallel volume of " << ddBlocks << " blocks, blockSize is " << blockSize << std::endl;
    
    numDDBlocks = embree::reduce_mul(ddBlocks);
    ddBlock     = new DDBlock[numDDBlocks];

    int blockID = 0;
    for (int iz=0;iz<ddBlocks.z;iz++)
      for (int iy=0;iy<ddBlocks.y;iy++)
        for (int ix=0;ix<ddBlocks.x;ix++, blockID++) {
          ddBlock[blockID].owner = -1;
          ddBlock[blockID].mine  = true;
          ddBlock[blockID].domain.lower = vec3i(ix,iy,iz) * blockSize;
          ddBlock[blockID].domain.upper = min(ddBlock[blockID].domain.lower+blockSize,dimensions);
          ddBlock[blockID].bounds.lower = vec3f(ddBlock[blockID].domain.lower) / vec3f(dimensions);
          ddBlock[blockID].bounds.upper = vec3f(ddBlock[blockID].domain.upper) / vec3f(dimensions);
          ddBlock[blockID].ispcVolumeHandle = NULL;
        }

    // Create an ISPC BlockBrickedVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::DDBVolume_create(this,                                                             
                                            (int)getVoxelType(), 
                                            (const ispc::vec3i&)dimensions,
                                            (const ispc::vec3i&)ddBlocks,
                                            (const ispc::vec3i&)blockSize,
                                            (ispc::DDBVolumeBlock*)ddBlock);
    if (!ispcEquivalent) 
      throw std::runtime_error("could not create create data distributed volume");
  }

  // A volume type with internal data-distribution. needs a renderer
  // that is capable of data-parallel rendering!
  OSP_REGISTER_VOLUME(DataDistributedBlockedVolume, data_distributed_volume);

} // ::ospray


