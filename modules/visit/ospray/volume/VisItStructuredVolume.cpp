// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "common/Data.h"
#include "volume/VisItStructuredVolume.h"
#include "VisItGridAccelerator_ispc.h"
#include "VisItStructuredVolume_ispc.h"

namespace ospray {

  std::string VisItStructuredVolume::toString() const
  {
    return("ospray::VisItStructuredVolume<" + voxelType + ">");
  }

  void VisItStructuredVolume::commit()
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


    this->scaleFactor = getParam3f("scaleFactor", vec3f(-1.f));

    ispc::VisItStructuredVolume_setGridOrigin(ispcEquivalent,
                                         (const ispc::vec3f&)this->gridOrigin);
    ispc::VisItStructuredVolume_setGridSpacing(ispcEquivalent,
                                         (const ispc::vec3f&)this->gridSpacing);

    // Complete volume initialization (only on first commit).
    if (!finished) {
      finish();
      finished = true;
    }
  }

  bool VisItStructuredVolume::scaleRegion(const void *source, void *&out,
                                     vec3i &regionSize, vec3i &regionCoords)
  {
    this->scaleFactor = getParam3f("scaleFactor", vec3f(-1.f));
    const bool upsampling = scaleFactor.x > 0
                            && scaleFactor.y > 0
                            && scaleFactor.z > 0;
    vec3i scaledRegionSize = vec3i(scaleFactor * vec3f(regionSize));
    vec3i scaledRegionCoords = vec3i(scaleFactor * vec3f(regionCoords));

    if (upsampling) {
      if (voxelType == "uchar") {
        out = malloc(sizeof(unsigned char) * size_t(scaledRegionSize.x) *
            size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((unsigned char *)source, (unsigned char *)out,
                       regionSize, scaledRegionSize);
      }
      else if (voxelType == "short") {
        out = malloc(sizeof(short) * size_t(scaledRegionSize.x) *
            size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((unsigned short *)source, (unsigned short *)out,
                       regionSize, scaledRegionSize);
      }
      else if (voxelType == "ushort") {
        out = malloc(sizeof(unsigned short) * size_t(scaledRegionSize.x) *
            size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((unsigned short *)source, (unsigned short *)out,
                       regionSize, scaledRegionSize);
      }
      else if (voxelType == "float") {
        out = malloc(sizeof(float) * size_t(scaledRegionSize.x) *
            size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((float *)source, (float *)out, regionSize,
                       scaledRegionSize);
      }
      else if (voxelType == "double") {
        out = malloc(sizeof(double) * size_t(scaledRegionSize.x) *
            size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((double *)source, (double *)out, regionSize,
                       scaledRegionSize);
      }
      regionSize = scaledRegionSize;
      regionCoords = scaledRegionCoords;
    }
    return upsampling;
  }

  void VisItStructuredVolume::buildAccelerator()
  {
    // Create instance of volume accelerator.
    void *accel = ispc::VisItStructuredVolume_createAccelerator(ispcEquivalent);

    vec3i brickCount;
    brickCount.x = ispc::VisItGridAccelerator_getBrickCount_x(accel);
    brickCount.y = ispc::VisItGridAccelerator_getBrickCount_y(accel);
    brickCount.z = ispc::VisItGridAccelerator_getBrickCount_z(accel);

    // Build volume accelerator.
    const int NTASKS = brickCount.x * brickCount.y * brickCount.z;
    tasking::parallel_for(NTASKS, [&](int taskIndex){
      ispc::VisItGridAccelerator_buildAccelerator(ispcEquivalent, taskIndex);
    });
  }

  void VisItStructuredVolume::finish()
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

  OSPDataType VisItStructuredVolume::getVoxelType()
  {
    return finished ? typeForString(getParamString("voxelType", "unspecified")):
                      typeForString(voxelType.c_str());
  }

} // ::ospray

