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
    NOTIMPLEMENTED;
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
    
    PING;

    return 0;
  }

  void DataDistributedBlockedVolume::createEquivalentISPC()
  {
    if (ispcEquivalent != NULL) return;

    ispcEquivalent = ispc::DDBVolume_create(this,(ispc::vec3i&)dimensions);
    if (!ispcEquivalent) 
      throw std::runtime_error("could not create create data distributed volume");
  }

  // A volume type with internal data-distribution. needs a renderer
  // that is capable of data-parallel rendering!
  OSP_REGISTER_VOLUME(DataDistributedBlockedVolume, data_distributed_volume);

} // ::ospray


