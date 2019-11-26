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
#include <iostream>
// stl
#include <random>

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Curves : public detail::Builder
    {
      Curves()           = default;
      ~Curves() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;

      std::string curveBasis;
    };

    static std::vector<vec4f> points = {
      {-1.0f, 0.0f, -2.f, 0.2f},
      {0.0f, -1.0f, 0.0f, 0.2f},
      {1.0f, 0.0f, 2.f, 0.2f},
      {-1.0f, 0.0f, 2.f, 0.2f},
      {0.0f, 1.0f, 0.0f, 0.6f},
      {1.0f, 0.0f, -2.f, 0.2f},
      {-1.0f, 0.0f, -2.f, 0.2f},
      {0.0f, -1.0f, 0.0f, 0.2f},
      {1.0f, 0.0f, 2.f, 0.2f}
    };
    static std::vector<unsigned int> indices = {0, 1, 2, 3, 4, 5};

    void Curves::commit()
    {
      Builder::commit();

      curveBasis = getParam<std::string>("curveBasis", "bspline");
    }

    cpp::Group Curves::buildGroup() const
    {
      std::vector<cpp::GeometricModel> geometricModels;
      std::mt19937 gen(randomSeed);
      std::uniform_real_distribution<float> colorDistribution(0.1f, 1.0f);
      std::vector<vec4f> s_colors(points.size());
      
      cpp::Geometry geom("curves");

      if(curveBasis == "hermite") {
        geom.setParam("type", int(OSP_ROUND));
        geom.setParam("basis", int(OSP_HERMITE));
        std::vector<vec4f> tangents;
        for(auto iter = points.begin(); iter != points.end() - 1; ++iter) {
          const vec4f pointTangent = *(iter+1) - *iter;
          tangents.push_back(pointTangent);
        }
        geom.setParam("vertex.position_radius", cpp::Data(points));
        geom.setParam("vertex.tangent", cpp::Data(tangents));
      } else if (curveBasis == "catmull-rom") {
        geom.setParam("type", int(OSP_ROUND));
        geom.setParam("basis", int(OSP_CATMULL_ROM));
        geom.setParam("vertex.position_radius", cpp::Data(points));
      } else if (curveBasis == "linear") {
        geom.setParam("radius", 0.1f);
        geom.setParam("vertex.position", cpp::Data(points.size(), sizeof(vec4f), (vec3f*)points.data()));
      } else {
        geom.setParam("type", int(OSP_ROUND));
        geom.setParam("basis", int(OSP_BSPLINE));
        geom.setParam("vertex.position_radius", cpp::Data(points));
     }

      for (auto &c : s_colors) {
        c.x = colorDistribution(gen);
        c.y = colorDistribution(gen);
        c.z = colorDistribution(gen);
        c.w = colorDistribution(gen);
      }

      geom.setParam("vertex.color", cpp::Data(s_colors));
      
      geom.setParam("index", cpp::Data(indices));
      geom.commit();

      cpp::Material mat(rendererType, "ThinGlass");
      mat.setParam("attenuationDistance", 1.0f);
      mat.commit();

      cpp::GeometricModel model(geom);
      model.setParam("material", cpp::Data(mat));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Curves, curves);

  }  // namespace testing
}  // namespace ospray
