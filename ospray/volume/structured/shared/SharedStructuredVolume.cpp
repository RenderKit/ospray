// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

  SharedStructuredVolume::~SharedStructuredVolume()
  {
    // No longer listen for changes to voxelData.
    if(voxelData) voxelData->unregisterListener(this);
  }

  std::string SharedStructuredVolume::toString() const
  {
    return("ospray::SharedStructuredVolume<" + voxelType + ">");
  }

  void SharedStructuredVolume::commit()
  {
    // Create the equivalent ISPC volume container.
    if (ispcEquivalent == nullptr) createEquivalentISPC();

    // StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  int SharedStructuredVolume::setRegion(const void *source,
                                        const vec3i &index,
                                        const vec3i &count)
  {
    if (getIE() == nullptr)
      createEquivalentISPC();
    switch (getVoxelType()) {
    case OSP_UCHAR:
      ispc::SharedStructuredVolume_setRegion_uint8(getIE(),source,
                                                   (const ispc::vec3i&)index,
                                                   (const ispc::vec3i&)count);
      break;
    case OSP_SHORT:
      ispc::SharedStructuredVolume_setRegion_int16(getIE(),source,
                                                   (const ispc::vec3i&)index,
                                                   (const ispc::vec3i&)count);
      break;
    case OSP_USHORT:
      ispc::SharedStructuredVolume_setRegion_uint16(getIE(),source,
                                                   (const ispc::vec3i&)index,
                                                   (const ispc::vec3i&)count);
      break;
    case OSP_FLOAT:
      ispc::SharedStructuredVolume_setRegion_float(getIE(),source,
                                                   (const ispc::vec3i&)index,
                                                   (const ispc::vec3i&)count);
      break;
    case OSP_DOUBLE:
      ispc::SharedStructuredVolume_setRegion_double(getIE(),source,
                                                   (const ispc::vec3i&)index,
                                                   (const ispc::vec3i&)count);
      break;
    default:
      throw std::runtime_error("SharedStructuredVolume::setRegion() not "
                               "support for volumes of voxel type '"
                               + voxelType + "'");
    }
    return true;
  }

  void SharedStructuredVolume::createEquivalentISPC()
  {
    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");
    const OSPDataType ospVoxelType = getVoxelType();
    if(ospVoxelType == OSP_UNKNOWN)
      throw std::runtime_error("unrecognized voxel type");

    // Get the volume dimensions.
    vec3i dimensions = getParam3i("dimensions", vec3i(0));
    if(reduce_min(dimensions) <= 0)
      throw std::runtime_error("invalid volume dimensions");

    // Get the voxel data.
    voxelData = (Data *)getParamObject("voxelData", nullptr);

    if (voxelData && !(voxelData->flags & OSP_DATA_SHARED_BUFFER)) {
      postStatusMsg(1)
        << "WARNING: The voxel data buffer was not created with"
        << " the OSP_DATA_SHARED_BUFFER flag; Use another volume type"
        << " (e.g. BlockBrickedVolume) for better performance";
    }

    // The voxel count.
    size_t voxelCount = (size_t)dimensions.x *
                        (size_t)dimensions.y *
                        (size_t)dimensions.z;
    allocatedVoxelData = (voxelData == nullptr) ?
                         malloc(voxelCount*sizeOf(ospVoxelType)) : nullptr;

    // Create an ISPC SharedStructuredVolume object and assign
    // type-specific function pointers.
    ispcEquivalent
      = ispc::SharedStructuredVolume_createInstance
      (this,ospVoxelType,(const ispc::vec3i &)dimensions,
       voxelData?voxelData->data:allocatedVoxelData);

    // Listen for changes to voxelData.
    if (voxelData)
      voxelData->registerListener(this);
  }

  void SharedStructuredVolume::dependencyGotChanged(ManagedObject *object)
  {
    // Rebuild volume accelerator when voxelData is committed.
    if(object == voxelData && ispcEquivalent)
      StructuredVolume::buildAccelerator();
  }

  // A volume type with XYZ storage order. The voxel data is provided by the
  // application via a shared data buffer.
  OSP_REGISTER_VOLUME(SharedStructuredVolume, shared_structured_volume);

} // ::ospray
