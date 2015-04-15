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
    vec3f gridOrigin = getParam3f("gridOrigin", vec3f(0.f));
    ispc::StructuredVolume_setGridOrigin(ispcEquivalent, (const ispc::vec3f &) gridOrigin);

    //! Set the grid spacing, default to (1,1,1).
    vec3f gridSpacing = getParam3f("gridSpacing", vec3f(1.f));
    ispc::StructuredVolume_setGridSpacing(ispcEquivalent, (const ispc::vec3f &) gridSpacing);

    //! Complete volume initialization (only on first commit).
    if (!finished) {
      finish();
      finished = true;
    }
  }

  void StructuredVolume::finish()
  {
    Volume::finish();

    //! Make the voxel value range visible to the application.
    if (findParam("voxelRange") == NULL)
      set("voxelRange", voxelRange);
    else
      voxelRange = getParam2f("voxelRange", voxelRange);

    //! Complete volume initialization.
    ispc::StructuredVolume_finish(ispcEquivalent);
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

} // ::ospray

