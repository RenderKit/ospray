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

// ospray
#include "StructuredVolume.h"
#include "../common/Data.h"
#include "GridAccelerator_ispc.h"
#include "StructuredVolume_ispc.h"
#include "Volume_ispc.h"

namespace ospray {

  StructuredVolume::~StructuredVolume()
  {
    if (ispcEquivalent)
      ispc::StructuredVolume_destroy(ispcEquivalent);
  }

  std::string StructuredVolume::toString() const
  {
    return ("ospray::StructuredVolume<" + voxelType + ">");
  }

  void StructuredVolume::commit()
  {
    Volume::commit();

    this->gridOrigin = getParam3f("gridOrigin", vec3f(0.f));
    this->dimensions = getParam3i("dimensions", vec3i(0));

    if (reduce_min(this->dimensions) <= 0)
      throw std::runtime_error("invalid volume dimensions!");

    this->gridSpacing = getParam3f("gridSpacing", vec3f(1.f));
    this->scaleFactor = getParam3f("scaleFactor", vec3f(-1.f));

    ispc::StructuredVolume_set(getIE(),
                               (const ispc::vec3f &)this->gridOrigin,
                               (const ispc::vec3f &)this->gridSpacing);

    if (!finished) {
      voxelRange = getParam2f("voxelRange", voxelRange);

      buildAccelerator();

      ispc::box3f boundingBox;
      ispc::Volume_getBoundingBox(&boundingBox, ispcEquivalent);
      bounds = (box3f &)boundingBox;

      finished = true;
    }
  }

  bool StructuredVolume::scaleRegion(const void *source,
                                     void *&out,
                                     vec3i &regionSize,
                                     vec3i &regionCoords)
  {
    this->scaleFactor = getParam3f("scaleFactor", vec3f(-1.f));
    const bool upsampling =
        scaleFactor.x > 0 && scaleFactor.y > 0 && scaleFactor.z > 0;
    vec3i scaledRegionSize   = vec3i(scaleFactor * vec3f(regionSize));
    vec3i scaledRegionCoords = vec3i(scaleFactor * vec3f(regionCoords));

    if (upsampling) {
      if (voxelType == "uchar") {
        out = malloc(sizeof(unsigned char) * size_t(scaledRegionSize.x) *
                     size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((const unsigned char *)source,
                       (unsigned char *)out,
                       regionSize,
                       scaledRegionSize);
      } else if (voxelType == "short") {
        out = malloc(sizeof(short) * size_t(scaledRegionSize.x) *
                     size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((const unsigned short *)source,
                       (unsigned short *)out,
                       regionSize,
                       scaledRegionSize);
      } else if (voxelType == "ushort") {
        out = malloc(sizeof(unsigned short) * size_t(scaledRegionSize.x) *
                     size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((const unsigned short *)source,
                       (unsigned short *)out,
                       regionSize,
                       scaledRegionSize);
      } else if (voxelType == "float") {
        out = malloc(sizeof(float) * size_t(scaledRegionSize.x) *
                     size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion(
            (const float *)source, (float *)out, regionSize, scaledRegionSize);
      } else if (voxelType == "double") {
        out = malloc(sizeof(double) * size_t(scaledRegionSize.x) *
                     size_t(scaledRegionSize.y) * size_t(scaledRegionSize.z));
        upsampleRegion((const double *)source,
                       (double *)out,
                       regionSize,
                       scaledRegionSize);
      }
      regionSize   = scaledRegionSize;
      regionCoords = scaledRegionCoords;
    }

    return upsampling;
  }

  void StructuredVolume::buildAccelerator()
  {
    void *accel = ispc::StructuredVolume_createAccelerator(ispcEquivalent);

    vec3i brickCount;
    brickCount.x = ispc::GridAccelerator_getBrickCount_x(accel);
    brickCount.y = ispc::GridAccelerator_getBrickCount_y(accel);
    brickCount.z = ispc::GridAccelerator_getBrickCount_z(accel);

    const int NTASKS = brickCount.x * brickCount.y * brickCount.z;
    tasking::parallel_for(NTASKS, [&](int taskIndex) {
      ispc::GridAccelerator_buildAccelerator(ispcEquivalent, taskIndex);
    });
  }

  OSPDataType StructuredVolume::getVoxelType()
  {
    return finished ? typeForString(getParamString("voxelType", "unspecified"))
                    : typeForString(voxelType.c_str());
  }

}  // namespace ospray
