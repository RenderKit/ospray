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
#include "ospray/common/Core.h"
#include "ospray/volume/BlockBrickedVolume.h"
#include "BlockBrickedVolume_ispc.h"
// std
#include <cassert>

namespace ospray {

  void BlockBrickedVolume::commit()
  {
    //! The ISPC volume container should already exist.
    exitOnCondition(ispcEquivalent == NULL, 
                    "the volume data must be set via ospSetRegion() "
                    "prior to commit for this volume type");

    PRINT(dimensions);
    //! StructuredVolume commit actions.
    StructuredVolume::commit();
    PRINT(dimensions);
  }

  int BlockBrickedVolume::setRegion(const void *source, const vec3i &index, const vec3i &count)
  {
    //! Create the equivalent ISPC volume container and allocate memory for voxel data.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    //! Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "float" && findParam("voxelRange") == NULL) 
      computeVoxelRange((float *) source, size_t(count.x) * count.y * count.z);

    //! Compute the voxel value range for unsigned byte voxels if none was previously specified.
    if (voxelType == "uchar" && findParam("voxelRange") == NULL) 
      computeVoxelRange((unsigned char *) source, size_t(count.x) * count.y * count.z);

    //! Copy voxel data into the volume.
    ispc::BlockBrickedVolume_setRegion(ispcEquivalent, source, (const ispc::vec3i &) index, (const ispc::vec3i &) count);

    //! DO ME: this return value should indicate the success or failure of memory allocation in ISPC and a range check.
    return true;
  }

  void BlockBrickedVolume::createEquivalentISPC() 
  {
    //! Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, 
                    "unrecognized voxel type (must be set before calling ospSetRegion())");

    //! Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions (must be set before calling ospSetRegion())");



#if EXP_DISTRIBUTED_VOLUME
    vec3i setRegionCoordOffset = vec3i(0);
    if (core::isMpiParallel()) {
      printf("creating structured volume in parallel MPI mode\n");

      vec3i compsDim(core::getWorkerCount(),1,1);
      PRINT(compsDim);
      PRINT(gridSpacing);
      PRINT(gridOrigin);
      PRINT(dimensions);
      vec3i compCoords(core::getWorkerRank(),0,0);
      
      box3i myRange;
      myRange.lower.x = (dimensions.x * compCoords.x) / compsDim.x;
      myRange.lower.y = (dimensions.y * compCoords.y) / compsDim.y;
      myRange.lower.z = (dimensions.z * compCoords.z) / compsDim.z;

      myRange.upper.x = (dimensions.x * (compCoords.x+1)) / compsDim.x;
      myRange.upper.y = (dimensions.y * (compCoords.y+1)) / compsDim.y;
      myRange.upper.z = (dimensions.z * (compCoords.z+1)) / compsDim.z;
      PRINT(myRange);

      this->logicalDimensions = dimensions;
      this->dimensions = myRange.upper - myRange.lower;
      this->myDomain = myRange;
      setRegionCoordOffset = myRange.lower;
    }
#endif


    //! Create an ISPC BlockBrickedVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::BlockBrickedVolume_createInstance((int)getVoxelType(),
                                                             (const ispc::vec3i &)this->dimensions, 
                                                             (const ispc::vec3i &)setRegionCoordOffset);
  }

} // ::ospray

