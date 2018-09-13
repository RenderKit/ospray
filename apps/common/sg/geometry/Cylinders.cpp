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

#include "Cylinders.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    Cylinders::Cylinders() : Geometry("cylinders") {}

    box3f Cylinders::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      if (hasChild("cylinders")) {
        auto cylinders = child("cylinders").nodeAs<DataBuffer>();

        auto *base = (byte_t*)cylinders->base();

        int cylinderBytes = 24;
        if (hasChild("bytes_per_cylinder"))
          cylinderBytes = child("bytes_per_cylinder").valueAs<int>();

        int offset_v0 = 0;
        if (hasChild("offset_v0"))
          offset_v0 = child("offset_v0").valueAs<int>();

        int offset_v1 = 12;
        if (hasChild("offset_v1"))
          offset_v1 = child("offset_v1").valueAs<int>();

        int offset_radius = -1;
        if (hasChild("offset_radius"))
          offset_radius = child("offset_radius").valueAs<int>();

        float radius = 0.01f;
        if (hasChild("radius"))
          radius = child("radius").valueAs<float>();

        for (size_t i = 0; i < cylinders->numBytes(); i += cylinderBytes) {
          vec3f &v0 = *(vec3f*)(base + i + offset_v0);
          vec3f &v1 = *(vec3f*)(base + i + offset_v1);
          if (offset_radius >= 0)
            radius = *(float*)(base + i + offset_radius);
          // TODO less conservative bbox
          box3f cylinderBounds(ospcommon::min(v0, v1) - radius,
                               ospcommon::max(v0, v1) + radius);
          bbox.extend(cylinderBounds);
        }
      }

      child("bounds") = bbox;

      return bbox;
    }

    OSP_REGISTER_SG_NODE(Cylinders);

  }// ::ospray::sg
}// ::ospray
