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

#pragma once

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
    StructuredVolume() {}

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

    //! Copy voxels into the volume at the given index.
    virtual void setRegion(const void *source, const vec3i &index, const vec3i &count) = 0;

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::StructuredVolume<" + voxelType + ">"); }

  protected:

    //! Volume size in voxels per dimension.
    vec3i volumeDimensions;

    //! Voxel type.
    std::string voxelType;

    //! Complete volume initialization.
    virtual void finish() = 0;

    //! Initialize the volume from memory.
    void getVolumeFromMemory();

    //! Update select parameters after the volume has been allocated and filled.
    virtual void updateEditableParameters() {}

  };

} // ::ospray

