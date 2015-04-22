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
#include "ospray/volume/SharedStructuredVolume.h"
#include "SharedStructuredVolume_ispc.h"
#include "ospray/common/Data.h"
// std
#include <cassert>

namespace ospray {

  void SharedStructuredVolume::commit()
  {
    //! Create the equivalent ISPC volume container.
    if (ispcEquivalent == NULL) createEquivalentISPC();

    //! StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  void SharedStructuredVolume::createEquivalentISPC() 
  {
    //! Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");  
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, "unrecognized voxel type");

    //! Get the volume dimensions.
    vec3i dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(dimensions) <= 0, "invalid volume dimensions");

    //! This volume type only supports 2GB volumes for now.
    size_t voxelSize = getVoxelType() == OSP_FLOAT ? 4 : 1;
    exitOnCondition(dimensions.x*dimensions.y*dimensions.z*voxelSize > (1L << 31), "this volume type currently only supports up to 2GB volumes");

    //! Get the voxel data.
    Data *voxelData = (Data *)getParamObject("voxelData", NULL);
    exitOnCondition(voxelData == NULL, "no voxel data provided");
    warnOnCondition(!(voxelData->flags & OSP_DATA_SHARED_BUFFER), "the voxel data buffer was not created with the OSP_DATA_SHARED_BUFFER flag; use another volume type (e.g. BlockBrickedVolume) for better performance");

    //! The voxel count.
    size_t voxelCount = (size_t)dimensions.x * dimensions.y * dimensions.z;
  
    //! Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "float" && findParam("voxelRange") == NULL) computeVoxelRange((float *)voxelData->data, voxelCount);

    //! Compute the voxel value range for unsigned byte voxels if none was previously specified.
    if (voxelType == "uchar" && findParam("voxelRange") == NULL) computeVoxelRange((unsigned char *)voxelData->data, voxelCount);

    //! Create an ISPC SharedStructuredVolume object and assign type-specific function pointers.
    ispcEquivalent = ispc::SharedStructuredVolume_createInstance((int)getVoxelType(), (const ispc::vec3i &)dimensions, voxelData->data);
  }

} // ::ospray
