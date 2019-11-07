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

#include "Builder.h"
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Boxes : public detail::Builder
    {
      Boxes()           = default;
      ~Boxes() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;

     private:
      vec3i dimensions{4};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Boxes::commit()
    {
      Builder::commit();

      dimensions = getParam<vec3i>("dimensions", vec3i(4));

      addPlane = false;
    }

    cpp::Group Boxes::buildGroup() const
    {
      cpp::Geometry boxGeometry("boxes");

      index_sequence_3D numBoxes(dimensions);

      std::vector<box3f> boxes;
      std::vector<vec4f> color;

      auto dim = reduce_max(dimensions);

      for (auto i : numBoxes) {
        auto i_f = static_cast<vec3f>(i);

        auto lower = i_f * 5.f;
        auto upper = lower + (0.75f * 5.f);
        boxes.emplace_back(lower, upper);

        auto box_color = (0.8f * i_f / dim) + 0.2f;
        color.emplace_back(box_color.x, box_color.y, box_color.z, 1.f);
      }

      boxGeometry.setParam("box", cpp::Data(boxes));
      boxGeometry.commit();

      cpp::GeometricModel model(boxGeometry);

      model.setParam("color", cpp::Data(color));

      cpp::Material material(rendererType, "OBJMaterial");
      material.commit();

      model.setParam("material", cpp::Data(material));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Boxes, boxes);

  }  // namespace testing
}  // namespace ospray
