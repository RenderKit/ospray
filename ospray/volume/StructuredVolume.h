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

// ospray
#include "ospray/volume/Volume.h"
// stl
#include <algorithm>
#include <string>

namespace ospray {

  /*! \brief A StructuredVolume is an abstraction for Volume subtypes
    in which the voxels are implicitly ordered. */
  /*! \detailed The actual memory layout, dimensionality, and source
    of samples are unknown to this class.  Subclasses may implement
    specific memory layouts, addressing precision, and voxel types.  A
    type string passed to Volume::createInstance() specifies a
    particular concrete implementation.  This type string must be
    registered in OSPRay proper, or in a loaded module via
    OSP_REGISTER_VOLUME.
  */
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

    //! Copy voxels into the volume at the given index
    /*! \returns 0 on error, any non-zero value indicates success */
    virtual int setRegion(const void *source_pointer, const vec3i &target_index, const vec3i &source_count) = 0;

  protected:

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC() = 0;

    //! Complete volume initialization (only on first commit).
    virtual void finish();

    //! Get the OSPDataType enum corresponding to the voxel type string.
    OSPDataType getVoxelType() const;

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    //! Compute the voxel value range for unsigned byte voxels.
    void computeVoxelRange(const unsigned char *source, const size_t &count);

    //! Compute the voxel value range for floating point voxels.
    void computeVoxelRange(const float *source, const size_t &count);

    //! Compute the voxel value range for double precision floating point voxels.
    void computeVoxelRange(const double *source, const size_t &count);
    
#endif


    //! build the accelerator - allows child class (data distributed) to avoid building..
    virtual void buildAccelerator();

    //! Volume size in voxels per dimension.
    vec3i dimensions;
    
    //! Grid origin.
    vec3f gridOrigin;
    
    //! Grid spacing in each dimension.
    vec3f gridSpacing;

    //! Indicate that the volume is fully initialized.
    bool finished;

  public:
    //! Voxel value range (will be computed if not provided as a parameter).
    vec2f voxelRange;

    //! Voxel type.
    std::string voxelType;
  };

} // ::ospray

