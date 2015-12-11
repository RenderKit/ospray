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
#include "ospray/common/Data.h"
#include "ospray/common/Core.h"
#include "ospray/common/Library.h"
#include "ospray/volume/StructuredVolume.h"
#include "GridAccelerator_ispc.h"
#include "StructuredVolume_ispc.h"

// stl
#include <map>

#ifdef OSPRAY_USE_TBB
# include <tbb/blocked_range.h>
# include <tbb/parallel_for.h>
#endif

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
    ispc::vec3i _brickCount = ispc::GridAccelerator_getBrickCount(accel);
    brickCount.x = _brickCount.x;
    brickCount.y = _brickCount.y;
    brickCount.z = _brickCount.z;

    // Build volume accelerator.
    const int NTASKS = brickCount.x * brickCount.y * brickCount.z;
#ifdef OSPRAY_USE_TBB
    tbb::parallel_for(tbb::blocked_range<int>(0, NTASKS),
                      [&](const tbb::blocked_range<int> &range) {
      for (int taskIndex = range.begin();
           taskIndex != range.end();
           ++taskIndex)
        ispc::GridAccelerator_buildAccelerator(ispcEquivalent, taskIndex);
    });
#else
#   pragma omp parallel for
    for (int taskIndex = 0; taskIndex < NTASKS; ++taskIndex) {
      ispc::GridAccelerator_buildAccelerator(ispcEquivalent, taskIndex);
    }
#endif
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

  OSPDataType StructuredVolume::getVoxelType() const
  {
    // Separate out the base type and vector width.
    char* kind = new char[voxelType.size()];
    unsigned int width = 1;
    sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

    OSPDataType res = OSP_UNKNOWN;

    // Unsigned 8-bit scalar integer.
    if (!strcmp(kind, "uchar") && width == 1)
      res = OSP_UCHAR;

    // Single precision scalar floating point.
    if (!strcmp(kind, "float") && width == 1)
      res = OSP_FLOAT;

    // Double precision scalar floating point.
    if (!strcmp(kind, "double") && width == 1)
      res = OSP_DOUBLE;

    delete[] kind;

    return res;
  }

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
  // Compute the voxel value range for unsigned byte voxels.
  void StructuredVolume::computeVoxelRange(const unsigned char *source,
                                           const size_t &count)
  {
#if 1
    const size_t blockSize = 1000000;
    int numBlocks = divRoundUp(count,blockSize);
    vec2f blockRange[numBlocks];
#pragma omp parallel for
    for (size_t i=0;i<numBlocks;i++) {
      size_t myBegin = i*blockSize;
      size_t myEnd   = std::min(myBegin+blockSize,count);
      vec2f myVoxelRange(source[myBegin]);

      for (size_t j=myBegin ; j < myEnd ; j++) {
        myVoxelRange.x = std::min(myVoxelRange.x, (float) source[j]);
        myVoxelRange.y = std::max(myVoxelRange.y, (float) source[j]);
      }

      blockRange[i] = myVoxelRange;
    }

    for (int i=0;i<numBlocks;i++) {
      voxelRange.x = std::min(voxelRange.x,blockRange[i].x);
      voxelRange.y = std::max(voxelRange.y,blockRange[i].y);
    }

#else
    for (size_t i=0 ; i < count ; i++) {
      voxelRange.x = std::min(voxelRange.x, (float) source[i]);
      voxelRange.y = std::max(voxelRange.y, (float) source[i]);
    }
#endif
  }

  // Compute the voxel value range for floating point voxels.
  void StructuredVolume::computeVoxelRange(const float *source,
                                           const size_t &count)
  {
#if 1
    const size_t blockSize = 1000000;
    int numBlocks = divRoundUp(count,blockSize);
    vec2f blockRange[numBlocks];
#pragma omp parallel for
    for (size_t i=0;i<numBlocks;i++) {
      size_t myBegin = i*blockSize;
      size_t myEnd   = std::min(myBegin+blockSize,count);
      vec2f myVoxelRange(source[myBegin]);

      for (size_t j=myBegin ; j < myEnd ; j++) {
        myVoxelRange.x = std::min(myVoxelRange.x, (float) source[j]);
        myVoxelRange.y = std::max(myVoxelRange.y, (float) source[j]);
      }

      blockRange[i] = myVoxelRange;
    }

    for (int i=0;i<numBlocks;i++) {
      voxelRange.x = std::min(voxelRange.x,blockRange[i].x);
      voxelRange.y = std::max(voxelRange.y,blockRange[i].y);
    }

#else
    for (size_t i=0 ; i < count ; i++) {
      voxelRange.x = std::min(voxelRange.x, source[i]);
      voxelRange.y = std::max(voxelRange.y, source[i]);
    }
#endif
  }

  // Compute the voxel value range for double precision floating point voxels.
  void StructuredVolume::computeVoxelRange(const double *source,
                                           const size_t &count)
  {
#if 1
    const size_t blockSize = 1000000;
    int numBlocks = divRoundUp(count,blockSize);
    vec2f blockRange[numBlocks];
#pragma omp parallel for
    for (size_t i=0;i<numBlocks;i++) {
      size_t myBegin = i*blockSize;
      size_t myEnd   = std::min(myBegin+blockSize,count);
      vec2f myVoxelRange(source[myBegin]);

      for (size_t j=myBegin ; j < myEnd ; j++) {
        myVoxelRange.x = std::min(myVoxelRange.x, (float) source[j]);
        myVoxelRange.y = std::max(myVoxelRange.y, (float) source[j]);
      }

      blockRange[i] = myVoxelRange;
    }

    for (int i=0;i<numBlocks;i++) {
      voxelRange.x = std::min(voxelRange.x,blockRange[i].x);
      voxelRange.y = std::max(voxelRange.y,blockRange[i].y);
    }

#else
    for (size_t i=0 ; i < count ; i++) {
      voxelRange.x = std::min(voxelRange.x, (float) source[i]);
      voxelRange.y = std::max(voxelRange.y, (float) source[i]);
    }
#endif
  }
#endif
} // ::ospray

