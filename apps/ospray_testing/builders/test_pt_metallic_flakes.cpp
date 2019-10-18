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

    struct PtMetallicFlakes : public detail::Builder
    {
      PtMetallicFlakes()           = default;
      ~PtMetallicFlakes() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
    };

    static cpp::Material makeMetallicFlakesMaterial(
        const std::string &rendererType,
        float flakeAmount,
        float flakeSpread)
    {
      cpp::Material mat(rendererType, "MetallicPaint");
      mat.setParam("baseColor", vec3f(1.f, 0.f, 0.f));
      mat.setParam("flakeAmount", flakeAmount);
      mat.setParam("flakeSpread", flakeSpread);
      mat.commit();

      return mat;
    }

    // Inlined definitions ////////////////////////////////////////////////////

    void PtMetallicFlakes::commit()
    {
      Builder::commit();
      addPlane = false;
    }

    cpp::Group PtMetallicFlakes::buildGroup() const
    {
      cpp::Geometry sphereGeometry("spheres");

      constexpr int dimSize = 5;

      index_sequence_2D numSpheres(dimSize);

      std::vector<vec3f> spheres;
      std::vector<uint8_t> index;
      std::vector<cpp::Material> materials;

      for (auto i : numSpheres) {
        auto i_f = static_cast<vec2f>(i);
        spheres.emplace_back(i_f.x, i_f.y, 0.f);
        materials.push_back(
            makeMetallicFlakesMaterial(rendererType,
                                       lerp(i_f.x / (dimSize - 1), 1.f, 0.f),
                                       lerp(i_f.y / (dimSize - 1), 1.f, 0.f)));
        index.push_back(numSpheres.flatten(i));
      }

      sphereGeometry.setParam("sphere.position", cpp::Data(spheres));
      sphereGeometry.setParam("radius", 0.4f);
      sphereGeometry.commit();

      cpp::GeometricModel model(sphereGeometry);

      model.setParam("material", cpp::Data(materials));
      model.setParam("index", cpp::Data(index));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(PtMetallicFlakes, test_pt_metallic_flakes);

  }  // namespace testing
}  // namespace ospray
