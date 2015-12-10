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
#include "StructuredVolume_ispc.h"
#include "ospray/common/Data.h"
// std
#include <cassert>

namespace ospray {

SharedStructuredVolume::SharedStructuredVolume() : voxelData(NULL) {}

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
    if (ispcEquivalent == NULL) createEquivalentISPC();

    // StructuredVolume commit actions.
    StructuredVolume::commit();
  }

  int SharedStructuredVolume::setRegion(const void *source, const vec3i &index, const vec3i &count)
  {
    exitOnCondition(true, "setRegion() not allowed on this volume type; "
                    "volume data must be provided via the voxelData parameter");
    return 0;
  }

  void SharedStructuredVolume::createEquivalentISPC()
  {
    // Get the voxel type.
    voxelType = getParamString("voxelType", "unspecified");
    exitOnCondition(getVoxelType() == OSP_UNKNOWN, "unrecognized voxel type");

    // Get the volume dimensions.
    vec3i dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(dimensions) <= 0, "invalid volume dimensions");

    // Get the voxel data.
    voxelData = (Data *)getParamObject("voxelData", NULL);
    exitOnCondition(voxelData == NULL, "no voxel data provided");
    warnOnCondition(!(voxelData->flags & OSP_DATA_SHARED_BUFFER),
                    "the voxel data buffer was not created with the OSP_DATA_SHARED_BUFFER flag; "
                    "use another volume type (e.g. BlockBrickedVolume) for better performance");

    // The voxel count.
    size_t voxelCount = (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    // Compute the voxel value range for unsigned byte voxels if none was previously specified.
    if (voxelType == "uchar" && findParam("voxelRange") == NULL)
      computeVoxelRange((unsigned char *)voxelData->data, voxelCount);

    // Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "float" && findParam("voxelRange") == NULL)
      computeVoxelRange((float *)voxelData->data, voxelCount);

    // Compute the voxel value range for float voxels if none was previously specified.
    if (voxelType == "double" && findParam("voxelRange") == NULL)
      computeVoxelRange((double *)voxelData->data, voxelCount);
#endif

    // Create an ISPC SharedStructuredVolume object and assign type-specific function pointers.
    int voxelType = (int)getVoxelType();
    ispcEquivalent
      = ispc::SharedStructuredVolume_createInstance(this,
                                                    voxelType,
                                                    (const ispc::vec3i &)dimensions,
                                                    voxelData->data);

    // Listen for changes to voxelData.
    voxelData->registerListener(this);
  }

  void SharedStructuredVolume::dependencyGotChanged(ManagedObject *object)
  {
    // Rebuild volume accelerator when voxelData is committed.
    if(object == voxelData && ispcEquivalent)
      StructuredVolume::buildAccelerator();
  }

  // A volume type with XYZ storage order. The voxel data is provided by the application via a shared data buffer.
  OSP_REGISTER_VOLUME(SharedStructuredVolume, shared_structured_volume);

} // ::ospray
