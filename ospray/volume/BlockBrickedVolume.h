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

#include "ospray/volume/StructuredVolume.h"

namespace ospray {

  //! \brief A concrete implementation of the StructuredVolume class
  //!  with 64-bit addressing in which the voxel data is laid out in
  //!  memory in multiple pages each in brick order.
  //!
  class BlockBrickedVolume : public StructuredVolume {
  public:

    ~BlockBrickedVolume();

    //! A string description of this class.
    std::string toString() const;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    void commit();

    //! Copy voxels into the volume at the given index (non-zero return value
    //!  indicates success).
    int setRegion(const void *source,
                  const vec3i &index,
                  const vec3i &count,
                  const vec3i &region);

  private:

    //! Create the equivalent ISPC volume container.
    void createEquivalentISPC();

  };

} // ::ospray

