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

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::StructuredVolume<" + voxelType + ">"); }

    //! Allocate storage and populate the volume, called through the OSPRay API.
    virtual void commit();

    //! Copy voxels into the volume at the given index (non-zero return value indicates success).
    virtual int setRegion(const void *source, const vec3i &index, const vec3i &count) = 0;

  protected:

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC() = 0;

    //! Complete volume initialization (only on first commit).
    virtual void finish();

    //! Get the OSPDataType enum corresponding to the voxel type string.
    OSPDataType getVoxelType() const;

    //! Compute the voxel value range for unsigned byte voxels.
    void computeVoxelRange(const unsigned char *source, const size_t &count);

    //! Compute the voxel value range for floating point voxels.
    void computeVoxelRange(const float *source, const size_t &count);

    //! Compute the voxel value range for double precision floating point voxels.
    void computeVoxelRange(const double *source, const size_t &count);

    //! Volume size in voxels per dimension.
    vec3i dimensions;
    
    //! Grid origin.
    vec3f gridOrigin;
    
    //! Grid spacing in each dimension.
    vec3f gridSpacing;

    //! Indicate that the volume is fully initialized.
    bool finished;

    //! Voxel value range (will be computed if not provided as a parameter).
    vec2f voxelRange;

    //! Voxel type.
    std::string voxelType;
  };

} // ::ospray

