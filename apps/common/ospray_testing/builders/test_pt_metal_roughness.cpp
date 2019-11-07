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

    struct PtMetalRoughness : public detail::Builder
    {
      PtMetalRoughness()           = default;
      ~PtMetalRoughness() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
    };

    static cpp::Material makeMetalMaterial(const std::string &rendererType,
                                           vec3f eta,
                                           vec3f k,
                                           float roughness)
    {
      cpp::Material mat(rendererType, "Metal");
      mat.setParam("eta", eta);
      mat.setParam("k", k);
      mat.setParam("roughness", roughness);
      mat.commit();

      return mat;
    }

    // Inlined definitions ////////////////////////////////////////////////////

    void PtMetalRoughness::commit()
    {
      Builder::commit();
      addPlane = false;
    }

    cpp::Group PtMetalRoughness::buildGroup() const
    {
      cpp::Geometry sphereGeometry("spheres");

      constexpr int dimSize = 5;

      index_sequence_2D numSpheres(dimSize);

      std::vector<vec3f> spheres;
      std::vector<uint8_t> index;

      for (auto i : numSpheres) {
        auto i_f = static_cast<vec2f>(i);
        spheres.emplace_back(i_f.x, i_f.y, 0.f);
        index.push_back(numSpheres.flatten(i));
      }

      sphereGeometry.setParam("sphere.position", cpp::Data(spheres));
      sphereGeometry.setParam("radius", 0.4f);
      sphereGeometry.commit();

      cpp::GeometricModel model(sphereGeometry);

      std::vector<cpp::Material> materials;

      // Silver
      for (int i = 0; i < dimSize; ++i) {
        materials.push_back(makeMetalMaterial(rendererType,
                                              vec3f(0.051, 0.043, 0.041),
                                              vec3f(5.3, 3.6, 2.3),
                                              float(i) / 4));
      }

      // Aluminium
      for (int i = 0; i < dimSize; ++i) {
        materials.push_back(makeMetalMaterial(rendererType,
                                              vec3f(1.5, 0.98, 0.6),
                                              vec3f(7.6, 6.6, 5.4),
                                              float(i) / 4));
      }

      // Gold
      for (int i = 0; i < dimSize; ++i) {
        materials.push_back(makeMetalMaterial(rendererType,
                                              vec3f(0.07, 0.37, 1.5),
                                              vec3f(3.7, 2.3, 1.7),
                                              float(i) / 4));
      }

      // Chromium
      for (int i = 0; i < dimSize; ++i) {
        materials.push_back(makeMetalMaterial(rendererType,
                                              vec3f(3.2, 3.1, 2.3),
                                              vec3f(3.3, 3.3, 3.1),
                                              float(i) / 4));
      }

      // Copper
      for (int i = 0; i < dimSize; ++i) {
        materials.push_back(makeMetalMaterial(rendererType,
                                              vec3f(0.1, 0.8, 1.1),
                                              vec3f(3.5, 2.5, 2.4),
                                              float(i) / 4));
      }

      model.setParam("material", cpp::Data(materials));
      model.setParam("index", cpp::Data(index));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(PtMetalRoughness, test_pt_metal_roughness);

  }  // namespace testing
}  // namespace ospray
