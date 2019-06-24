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

#include "Geometry.h"
// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/multidim_index_sequence.h"

#include <vector>

using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct Boxes : public Geometry
    {
      Boxes()           = default;
      ~Boxes() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingGeometry Boxes::createGeometry(
        const std::string &renderer_type) const
    {
      auto boxGeometry = ospNewGeometry("boxes");
      auto model       = ospNewGeometricModel(boxGeometry);

      const int dim = 4;

      index_sequence_3D numBoxes({dim, dim, dim});

      std::vector<box3f> boxes;
      std::vector<vec4f> color;
      box3f bounds;

      for (auto i : numBoxes) {
        auto i_f = static_cast<vec3f>(i);

        auto lower = i_f * 5.f;
        auto upper = lower + (0.75f * 5.f);
        boxes.emplace_back(lower, upper);

        bounds.extend(lower);
        bounds.extend(upper);

        auto box_color = (0.8f * i_f / dim) + 0.2f;
        color.emplace_back(box_color.x, box_color.y, box_color.z, 1.f);
      }

      auto boxData =
          ospNewData(numBoxes.total_indices(), OSP_BOX3F, boxes.data());

      ospSetData(boxGeometry, "boxes", boxData);
      ospRelease(boxData);

      ospCommit(boxGeometry);

      auto colorData =
          ospNewData(numBoxes.total_indices(), OSP_VEC4F, color.data());

      ospSetData(model, "color", colorData);
      ospRelease(colorData);

      // create OBJ material and assign to geometry
      OSPMaterial objMaterial =
          ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      ospCommit(objMaterial);

      ospSetObject(model, "material", objMaterial);
      ospRelease(objMaterial);

      ospCommit(model);

      OSPInstance instance = ospNewInstance();
      auto instances       = ospNewData(1, OSP_OBJECT, &model);
      ospSetData(instance, "geometries", instances);
      ospCommit(instance);
      ospRelease(instances);

      OSPTestingGeometry retval;
      retval.geometry = boxGeometry;
      retval.model    = model;
      retval.instance = instance;
      retval.bounds   = reinterpret_cast<osp_box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(Boxes, boxes);

  }  // namespace testing
}  // namespace ospray
