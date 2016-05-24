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
#include "ospray/common/Data.h"
#include "ospray/common/Core.h"
#include "ospray/common/Library.h"
#include "ospray/volume/StructuredVolume.h"
#include "GridAccelerator_ispc.h"
#include "StructuredVolume_ispc.h"

// stl
#include <map>

namespace ospray {

  StructuredVolume::StructuredVolume() :
    finished(false),
    voxelRange(FLT_MAX, -FLT_MAX)
  {
  }

  StructuredVolume::~StructuredVolume() {}

  std::string StructuredVolume::toString() const
  {
    return("ospray::StructuredVolume<" + voxelType + ">");
  }

  void StructuredVolume::commit()
  {
    // Some parameters can be changed after the volume has been allocated and
    // filled.
    updateEditableParameters();

    // Set the grid origin, default to (0,0,0).
    this->gridOrigin = getParam3f("gridOrigin", vec3f(0.f));

    // Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0,
                    "invalid volume dimensions");

    // Set the grid spacing, default to (1,1,1).
    this->gridSpacing = getParam3f("gridSpacing", vec3f(1.f));


    ispc::StructuredVolume_setGridOrigin(ispcEquivalent,
                                         (const ispc::vec3f&)this->gridOrigin);
    ispc::StructuredVolume_setGridSpacing(ispcEquivalent,
                                         (const ispc::vec3f&)this->gridSpacing);

    // Complete volume initialization (only on first commit).
    if (!finished) {
      finish();
      finished = true;
    }
  }

  void StructuredVolume::buildAccelerator()
  {
    // Create instance of volume accelerator.
    void *accel = ispc::StructuredVolume_createAccelerator(ispcEquivalent);

    vec3i brickCount;
    brickCount.x = ispc::GridAccelerator_getBrickCount_x(accel);
    brickCount.y = ispc::GridAccelerator_getBrickCount_y(accel);
    brickCount.z = ispc::GridAccelerator_getBrickCount_z(accel);

    // Build volume accelerator.
    const int NTASKS = brickCount.x * brickCount.y * brickCount.z;
    parallel_for(NTASKS, [&](int taskIndex){
      ispc::GridAccelerator_buildAccelerator(ispcEquivalent, taskIndex);
    });
  }

  void StructuredVolume::finish()
  {
    // Make the voxel value range visible to the application.
    if (findParam("voxelRange") == NULL)
      set("voxelRange", voxelRange);
    else
      voxelRange = getParam2f("voxelRange", voxelRange);

    buildAccelerator();

    // Volume finish actions.
    Volume::finish();
  }

  OSPDataType StructuredVolume::getVoxelType()
  {
    return finished ? typeForString(getParamString("voxelType", "unspecified")):
                      typeForString(voxelType.c_str());
  }

} // ::ospray

