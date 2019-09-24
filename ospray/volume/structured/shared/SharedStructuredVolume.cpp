// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "SharedStructuredVolume.h"
#include "SharedStructuredVolume_ispc.h"
#include "StructuredVolume_ispc.h"
#include "../../../common/Data.h"

namespace ospray {

  std::string SharedStructuredVolume::toString() const
  {
    return ("ospray::SharedStructuredVolume<" + stringFor(voxelType) + ">");
  }

  void SharedStructuredVolume::commit()
  {
    // Create the equivalent ISPC volume container.
    if (ispcEquivalent == nullptr) createEquivalentISPC();

    // StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  void SharedStructuredVolume::createEquivalentISPC()
  {
    // Get the voxel type.
    voxelType = (OSPDataType)getParam<int>("voxelType", OSP_UNKNOWN);
    if (voxelType == OSP_UNKNOWN) {
      throw std::runtime_error(
          "shared structured volume 'voxelType' has invalid type. Must be one "
          "of OSP_* types");
    }

    // Get the volume dimensions.
    vec3i dimensions = getParam<vec3i>("dimensions", vec3i(0));
    if(reduce_min(dimensions) <= 0) {
      throw std::runtime_error(
          "shared structured volume 'dimensions' has a zero or negative "
          "component");
    }

    // Get the voxel data.
    voxelData = (Data *)getParamObject("voxelData", nullptr);

    // The voxel count.
    size_t voxelCount = (size_t)dimensions.x *
                        (size_t)dimensions.y *
                        (size_t)dimensions.z;
    allocatedVoxelData = (voxelData == nullptr) ?
                         malloc(voxelCount*sizeOf(voxelType)) : nullptr;

    // Create an ISPC SharedStructuredVolume object and assign
    // type-specific function pointers.
    ispcEquivalent = ispc::SharedStructuredVolume_createInstance(this,
        voxelType,
        (const ispc::vec3i &)dimensions,
        voxelData ? voxelData->data() : allocatedVoxelData);
  }

  // A volume type with XYZ storage order. The voxel data is provided by the
  // application via a shared data buffer.
  OSP_REGISTER_VOLUME(SharedStructuredVolume, shared_structured_volume);

}  // namespace ospray
