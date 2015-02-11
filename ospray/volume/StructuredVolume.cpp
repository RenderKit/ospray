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
    updateEditableParameters();

    //! Complete volume initialization.
    if (!finished) finish(), finished = true;

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

