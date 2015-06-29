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
#include "ospray/common/Library.h"
#include "ospray/volume/StructuredVolume.h"
#include "StructuredVolume_ispc.h"
// stl
#include <map>

namespace ospray {

  void StructuredVolume::commit()
  {
    //! Some parameters can be changed after the volume has been allocated and filled.
    updateEditableParameters();

    //! Set the grid origin, default to (0,0,0).
    this->gridOrigin = getParam3f("gridOrigin", vec3f(0.f));
    ispc::StructuredVolume_setGridOrigin(ispcEquivalent, (const ispc::vec3f &) this->gridOrigin);

    //! Get the volume dimensions.
    this->dimensions = getParam3i("dimensions", vec3i(0));
    exitOnCondition(reduce_min(this->dimensions) <= 0, 
                    "invalid volume dimensions");

    //! Set the grid spacing, default to (1,1,1).
    this->gridSpacing = getParam3f("gridSpacing", vec3f(1.f));
    ispc::StructuredVolume_setGridSpacing(ispcEquivalent, (const ispc::vec3f &) this->gridSpacing);

    //! Complete volume initialization (only on first commit).
    if (!finished) {
      finish();
      finished = true;
    }
  }

  void StructuredVolume::finish()
  {
    //! Make the voxel value range visible to the application.
    if (findParam("voxelRange") == NULL)
      set("voxelRange", voxelRange);
    else
      voxelRange = getParam2f("voxelRange", voxelRange);

    //! Build volume accelerator.
    ispc::StructuredVolume_buildAccelerator(ispcEquivalent);

    //! Volume finish actions.
    Volume::finish();
  }

  OSPDataType StructuredVolume::getVoxelType() const
  {
    //! Separate out the base type and vector width.
    char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

    //! Single precision scalar floating point.
    if (!strcmp(kind, "float") && width == 1) return(OSP_FLOAT);

    //! Unsigned 8-bit scalar integer.
    if (!strcmp(kind, "uchar") && width == 1) return(OSP_UCHAR);

    //! Unknown voxel type.
    return OSP_UNKNOWN;
  }

  //! Compute the voxel value range for floating point voxels.
  void StructuredVolume::computeVoxelRange(const float *source, const size_t &count)
  { 
    for (size_t i=0 ; i < count ; i++) {
      voxelRange.x = std::min(voxelRange.x, source[i]);
      voxelRange.y = std::max(voxelRange.y, source[i]); 
    }
  }
  
  //! Compute the voxel value range for unsigned byte voxels.
  void StructuredVolume::computeVoxelRange(const unsigned char *source, const size_t &count)
  { 
    for (size_t i=0 ; i < count ; i++) {
      voxelRange.x = std::min(voxelRange.x, (float) source[i]);
      voxelRange.y = std::max(voxelRange.y, (float) source[i]); 
    }
  }
  
} // ::ospray

