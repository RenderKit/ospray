// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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
#include "ospray/common/Data.h"
#include "ospray/common/Library.h"
#include "ospray/volume/StructuredVolume.h"
// stl
#include <map>

namespace ospray {

  void StructuredVolume::commit() {

    //! Some parameters can be changed after the volume has been allocated and filled.
    if (ispcEquivalent != NULL) return(updateEditableParameters());

    //! Create the equivalent ISPC volume container and allocate memory for voxel data.
    createEquivalentISPC();

    //! Get a pointer to the source voxel buffer.
    const Data *voxelData = getParamData("voxelData", NULL);  exitOnCondition(voxelData == NULL, "no voxel data specified");

    //! The dimensions of the source voxel data and target volume must match.
    exitOnCondition(size_t(volumeDimensions.x) * volumeDimensions.y * volumeDimensions.z != voxelData->numItems, "unexpected source voxel data dimensions");

    //! The source and target voxel types must match.
    exitOnCondition(getVoxelType() != voxelData->type, "unexpected source voxel type");

    //! Compute voxel range for the volume, if not provided already.
    vec2f voxelRange = getParam2f("voxelRange", vec2f(FLT_MAX, -FLT_MAX));

    if(voxelRange == vec2f(FLT_MAX, -FLT_MAX)) {

      if(voxelData->type == OSP_FLOAT) {

        const float *data = (const float *)voxelData->data;

        for(size_t i=0; i<voxelData->numItems; i++) {
          voxelRange.x = std::min(voxelRange.x, data[i]);
          voxelRange.y = std::max(voxelRange.y, data[i]);
        }

        set("voxelRange", voxelRange);
      }
      else if(voxelData->type == OSP_UCHAR) {

        const uint8 *data = (const uint8 *)voxelData->data;

        for(size_t i=0; i<voxelData->numItems; i++) {
          voxelRange.x = std::min(voxelRange.x, (float)data[i]);
          voxelRange.y = std::max(voxelRange.y, (float)data[i]);
        }

        set("voxelRange", voxelRange);
      }
      else
        emitMessage("WARNING", "cannot compute voxel range for source voxel type");
    }

    //! Size of a volume slice in bytes.
    size_t sliceSizeInBytes = volumeDimensions.x * volumeDimensions.y * getVoxelSizeInBytes();

    //! Get a pointer to the raw voxel data.
    const uint8 *data = (const uint8 *) voxelData->data;

    //! Copy voxel data into the volume in slices to avoid overflow in ISPC offset calculations.
    for (size_t z=0 ; z < volumeDimensions.z ; z++) setRegion(&data[z * sliceSizeInBytes], vec3i(0, 0, z), vec3i(volumeDimensions.x, volumeDimensions.y, 1));

    //! Complete volume initialization.
    finish();

  }

  OSPDataType StructuredVolume::getVoxelType() const {

    //! Separate out the base type and vector width.
    char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

    //! Single precision scalar floating point.
    if (!strcmp(kind, "float") && width == 1) return(OSP_FLOAT);

    //! Unsigned 8-bit scalar integer.
    if (!strcmp(kind, "uchar") && width == 1) return(OSP_UCHAR);

    //! Unknown voxel type.
    return(OSP_UNKNOWN);

  }

  size_t StructuredVolume::getVoxelSizeInBytes() const {

    //! Separate out the base type and vector width.
    char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

    //! Unsigned character scalar and vector types.
    if (!strcmp(kind, "uchar") && width >= 1 && width <= 4) return(sizeof(unsigned char) * width);

    //! Floating point scalar and vector types.
    if (!strcmp(kind, "float") && width >= 1 && width <= 4) return(sizeof(float) * width);

    //! Unsigned integer scalar and vector types.
    if (!strcmp(kind, "uint") && width >= 1 && width <= 4) return(sizeof(unsigned int) * width);

    //! Signed integer scalar and vector types.
    if (!strcmp(kind, "int") && width >= 1 && width <= 4) return(sizeof(int) * width);  

    //! Unknown voxel type.
    return(0);

  }

} // ::ospray

