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

#pragma once

// ospray
#include "ospcommon/tasking/parallel_for.h"
#include "../Volume.h"

namespace ospray {

  /*! \brief A StructuredVolume is an abstraction for Volume subtypes
    in which the voxels are implicitly ordered. */
  /*! \detailed The actual memory layout, dimensionality, and source
    of samples are unknown to this class.  Subclasses may implement
    specific memory layouts, addressing precision, and voxel types.  A
    type string passed to Volume::createInstance() specifies a
    particular concrete implementation.  This type string must be
    registered in OSPRay proper, or in a loaded module via
    OSP_REGISTER_VOLUME. To place the volume in world coordinates,
    modify the gridOrigin and gridSpacing to translate and scale the volume.
  */
  class OSPRAY_SDK_INTERFACE StructuredVolume : public Volume {
  public:

    StructuredVolume() = default;
    virtual ~StructuredVolume() override;

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

    template<typename T>
    void upsampleRegion(const T *source,
                        T *out,
                        const vec3i &regionSize,
                        const vec3i &scaledRegionSize);

    /*! Scale up the region we're setting in ospSetRegion. Will return the
        scaled region size and coordinates through the regionSize and
        regionCoords passed for the unscaled region (from ospSetRegion),
        and return true if we actually upsampled the data. If we upsample the
        data the caller is responsible for calling free on the out data
        parameter to release the scaled volume data.
     */
    bool scaleRegion(const void *source, void *&out,
                     vec3i &regionSize, vec3i &regionCoords);

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
    bool finished {false};

    //! Voxel value range (will be computed if not provided as a parameter).
    vec2f voxelRange {FLT_MAX, -FLT_MAX};

    //! Voxel type.
    std::string voxelType;

    /*! Scale factor for the volume, mostly for internal use or data scaling
        benchmarking. Note that this must be set **before** calling
        'ospSetRegion' on the volume as the scaling is applied in that function.
     */
    vec3f scaleFactor;
  };

// Inlined member functions ///////////////////////////////////////////////////

  template<typename T>
  inline void StructuredVolume::upsampleRegion(const T *source,
                                               T *out,
                                               const vec3i &regionSize,
                                               const vec3i &scaledRegionSize)
  {
    for (int z = 0; z < scaledRegionSize.z; ++z) {
      const auto nTasks = scaledRegionSize.x * scaledRegionSize.y;

      tasking::parallel_for(nTasks, [&](int taskID) {
        int x = taskID % scaledRegionSize.x;
        int y = taskID / scaledRegionSize.x;
        const int idx =
            static_cast<int>(z / scaleFactor.z) * regionSize.x * regionSize.y
            + static_cast<int>(y / scaleFactor.y) * regionSize.x
            + static_cast<int>(x / scaleFactor.x);

        const auto outIdx = z * scaledRegionSize.y * scaledRegionSize.x
                            + y * scaledRegionSize.x + x;
        out[outIdx] = source[idx];
      });
    }
  }

} // ::ospray

