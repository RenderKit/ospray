// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "ospray/common/tasking/parallel_for.h"
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
    StructuredVolume();

    //! Destructor.
    virtual ~StructuredVolume();

    //! A string description of this class.
    virtual std::string toString() const override;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    virtual void commit() override;

    //! Copy voxels into the volume at the given index
    /*! \returns 0 on error, any non-zero value indicates success */
    virtual int setRegion(const void *source_pointer,
                          const vec3i &target_index,
                          const vec3i &source_count) override = 0;

  protected:

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC() = 0;

    //! Complete volume initialization (only on first commit).
    virtual void finish() override;

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
    template<typename T>
    void computeVoxelRange(const T* source, const size_t &count);
#endif

    //! build the accelerator - allows child class (data distributed) to avoid
    //! building..
    virtual void buildAccelerator();

    //! Get the OSPDataType enum corresponding to the voxel type string.
    OSPDataType getVoxelType();

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

// Inlined member functions ///////////////////////////////////////////////////

#ifndef OSPRAY_VOLUME_VOXELRANGE_IN_APP
  template<typename T>
  inline void StructuredVolume::computeVoxelRange(const T* source,
                                                  const size_t &count)
  {
    const size_t blockSize = 1000000;
    int numBlocks = divRoundUp(count, blockSize);
    vec2f* blockRange = (vec2f*)alloca(numBlocks*sizeof(vec2f));

    //NOTE(jda) - shouldn't this be a simple reduction (TBB/OMP)?
    parallel_for(numBlocks, [&](int taskIndex){
      size_t myBegin = taskIndex *blockSize;
      size_t myEnd   = std::min(myBegin+blockSize,count);
      vec2f myVoxelRange(source[myBegin]);

      for (size_t i = myBegin; i < myEnd ; i++) {
        myVoxelRange.x = std::min(myVoxelRange.x, (float)source[i]);
        myVoxelRange.y = std::max(myVoxelRange.y, (float)source[i]);
      }

      blockRange[taskIndex] = myVoxelRange;
    });

    for (int i = 0; i < numBlocks; i++) {
      voxelRange.x = std::min(voxelRange.x, blockRange[i].x);
      voxelRange.y = std::max(voxelRange.y, blockRange[i].y);
    }
  }
#endif

} // ::ospray

