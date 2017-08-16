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
// amr base
#include "AMRAccel.h"

namespace ospray {
  namespace amr {

    /*! abstraction for an object that performs scalar volume sampling */
    struct VolumeSampler
    {
      /*! compute scalar field value at given location */
      virtual float sample(const vec3f &v) = 0;
      virtual float sampleLevel(const vec3f &v, float& width) = 0;
      /*! compute gradient at given location */
      virtual vec3f gradient(const vec3f &v) = 0;
    };
    
    /*! the actual ospray volume object */
    struct AMRVolume : public ospray::Volume {
      //! default constructor
      AMRVolume();

      //! \brief common function to help printf-debugging 
      virtual std::string toString() const { return "ospray::amr::AMR"; }
      
      //! Allocate storage and populate the volume.
      virtual void commit();

      /*! Copy voxels into the volume at the given index (non-zero
        return value indicates success). */
      virtual int setRegion(const void *source, const vec3i &index, const vec3i &count);

      AMRData  *data;
      AMRAccel *accel;
      VolumeSampler *sampler;
      
      Ref<Data> brickInfoData;
      Ref<Data> brickDataData;

      //! Voxel value range (will be computed if not provided as a parameter).
      vec2f voxelRange {FLT_MAX, -FLT_MAX};

      /*! Scale factor for the volume, mostly for internal use or data scaling
        benchmarking. Note that this must be set **before** calling
        'ospSetRegion' on the volume as the scaling is applied in that function.
      */
      vec3f scaleFactor {1};
    };

  } // ::ospray::amr
} // ::ospray

