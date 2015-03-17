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

#pragma once

#include <algorithm>
#include <string>
#include "ospray/volume/Volume.h"

namespace ospray {

  //! \brief A StructuredVolume is an abstraction for Volume subtypes
  //!  in which the voxels are implicitly ordered.
  //!
  //!  The actual memory layout, dimensionality, and source of samples
  //!  are unknown to this class.  Subclasses may implement specific
  //!  memory layouts, addressing precision, and voxel types.  A type
  //!  string passed to Volume::createInstance() specifies a particular
  //!  concrete implementation.  This type string must be registered
  //!  in OSPRay proper, or in a loaded module via OSP_REGISTER_VOLUME.
  //!
  class StructuredVolume : public Volume {
  public:

    //! Constructor.
    StructuredVolume() : finished(false), voxelRange(FLT_MAX, -FLT_MAX) {}

    //! Destructor.
    virtual ~StructuredVolume() {};

    //! Allocate storage and populate the volume, called through the OSPRay API.
    virtual void commit();

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC() = 0;

    //! Volume size in voxels per dimension.
    const vec3i &getDimensions() const { return(volumeDimensions); }

    //! Voxel size in bytes.
    size_t getVoxelSizeInBytes() const;

    //! Get the OSPDataType enum corresponding to the voxel type string.
    OSPDataType getVoxelType() const;

    //! Copy voxels into the volume at the given index (non-zero return value indicates success).
    virtual int setRegion(const void *source, const vec3i &index, const vec3i &count) = 0;

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::StructuredVolume<" + voxelType + ">"); }

  protected:

    //! Indicate that the volume is fully initialized.
    bool finished;

    //! Volume size in voxels per dimension.
    vec3i volumeDimensions;

    //! Voxel value range.
    vec2f voxelRange;

    //! Voxel type.
    std::string voxelType;

    //! Complete volume initialization.
    virtual void finish() = 0;

    //! Update select parameters after the volume has been allocated and filled.
    virtual void updateEditableParameters() {}

    //! Compute the voxel value range for floating point voxels.
    inline void computeVoxelRange(const float *source, const size_t &count)
      { for (size_t i=0 ; i < count ; i++) voxelRange.x = std::min(voxelRange.x, source[i]), voxelRange.y = std::max(voxelRange.y, source[i]); }

    //! Compute the voxel value range for unsigned byte voxels.
    inline void computeVoxelRange(const unsigned char *source, const size_t &count)
      { for (size_t i=0 ; i < count ; i++) voxelRange.x = std::min(voxelRange.x, (float) source[i]), voxelRange.y = std::max(voxelRange.y, (float) source[i]); }

  };

} // ::ospray

