// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "Spheres.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    Spheres::Spheres() : Geometry("spheres") {}

    box3f Spheres::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      if (hasChild("spheres")) {
        auto spheres = child("spheres").nodeAs<DataBuffer>();

        auto *base = (byte_t*)spheres->base();

        int sphereBytes = 16;
        if (hasChild("bytes_per_sphere"))
          sphereBytes = child("bytes_per_sphere").valueAs<int>();

        int offset_center = 0;
        if (hasChild("offset_center"))
          offset_center = child("offset_center").valueAs<int>();

        int offset_radius = -1;
        if (hasChild("offset_radius"))
          offset_radius = child("offset_radius").valueAs<int>();

        float radius = 0.01f;
        if (hasChild("radius"))
          radius = child("radius").valueAs<float>();

        for (size_t i = 0; i < spheres->numBytes(); i += sphereBytes) {
          vec3f &center = *(vec3f*)(base + i + offset_center);
          if (offset_radius >= 0)
            radius = *(float*)(base + i + offset_radius);
          box3f sphereBounds(center - radius, center + radius);
          bbox.extend(sphereBounds);
        }
      }

      child("bounds") = bbox;

      return bbox;
    }

    OSP_REGISTER_SG_NODE(Spheres);

  }// ::ospray::sg
}// ::ospray
