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
// stl
#include <random>
// ospcommon
#include "ospcommon/math/box.h"

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Curves : public detail::Builder
    {
      Curves()           = default;
      ~Curves() override = default;

      cpp::Group buildGroup() const override;

     private:
      void kleinBottle(std::vector<vec4f> &points,
                       std::vector<int> &indices,
                       std::vector<vec4f> &colors) const;
      void embreeTutorialCurve(std::vector<vec4f> &points,
                               std::vector<int> &indices,
                               std::vector<vec4f> &colors) const;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Curves::kleinBottle(std::vector<vec4f> &points,
                             std::vector<int> &indices,
                             std::vector<vec4f> &colors) const
    {
      // spout
      points.push_back(vec4f(0.0f, 0.0f, 0.0f, 0.1f));
      points.push_back(vec4f(0.0f, 1.5f, 0.0f, 0.05f));
      points.push_back(vec4f(-.3f, 1.65f, 0.0f, 0.05f));
      points.push_back(vec4f(-.6f, 1.5f, 0.0f, 0.1f));
      // handle
      points.push_back(vec4f(-.3f, 0.0f, 0.0f, 0.1f));
      // base
      points.push_back(vec4f(0.0f, -.5f, 0.0f, 0.3f));
      points.push_back(vec4f(0.0f, -.65f, 0.0f, 0.3f));
      points.push_back(vec4f(0.0f, 0.0f, 0.0f, 0.3f));
      points.push_back(vec4f(0.0f, 0.0f, 0.0f, 0.3f));
      // neck
      points.push_back(vec4f(0.0f, 1.5f, 0.0f, 0.05));
      points.push_back(vec4f(-.3f, 1.65f, 0.0f, 0.05f));

      for (int i = 0; i < 8; i++) {
        indices.push_back(i);
        colors.push_back(vec4f(1.0f, 0.0f, 0.0f, 0.0f));
      }
    }

    // from Embree's curve_geometry tutorial
    void Curves::embreeTutorialCurve(std::vector<vec4f> &points,
                                     std::vector<int> &indices,
                                     std::vector<vec4f> &colors) const
    {
      points.push_back(vec4f(-1.0f, 0.0f, -2.f, 0.2f));
      points.push_back(vec4f(0.0f, -1.0f, 0.0f, 0.2f));
      points.push_back(vec4f(1.0f, 0.0f, 2.f, 0.2f));
      points.push_back(vec4f(-1.0f, 0.0f, 2.f, 0.2f));
      points.push_back(vec4f(0.0f, 1.0f, 0.0f, 0.6f));
      points.push_back(vec4f(1.0f, 0.0f, -2.f, 0.2f));
      points.push_back(vec4f(-1.0f, 0.0f, -2.f, 0.2f));
      points.push_back(vec4f(0.0f, -1.0f, 0.0f, 0.2f));
      points.push_back(vec4f(1.0f, 0.0f, 2.f, 0.2f));

      indices.push_back(0);
      indices.push_back(1);
      indices.push_back(2);
      indices.push_back(3);
      indices.push_back(4);
      indices.push_back(5);

      colors.push_back(vec4f(1.0f, 0.0f, 0.0f, 0.0f));
      colors.push_back(vec4f(1.0f, 1.0f, 0.0f, 0.0f));
      colors.push_back(vec4f(0.0f, 1.0f, 0.0f, 0.0f));
      colors.push_back(vec4f(0.0f, 1.0f, 1.0f, 0.0f));
      colors.push_back(vec4f(0.0f, 0.0f, 1.0f, 0.0f));
      colors.push_back(vec4f(1.0f, 0.0f, 1.0f, 0.0f));
    }

    cpp::Group Curves::buildGroup() const
    {
      cpp::Geometry geom("curves");

      geom.setParam("type", int(OSP_ROUND));
      geom.setParam("basis", int(OSP_BSPLINE));

      std::vector<vec4f> points;
      std::vector<int> indices;
      std::vector<vec4f> colors;

#if 0
      kleinBottle(points, indices, colors);
#else
      embreeTutorialCurve(points, indices, colors);
#endif

      geom.setParam("vertex.position",
                    cpp::Data(points.size(), OSP_VEC4F, points.data()));
      geom.setParam("index",
                    cpp::Data(indices.size(), OSP_UINT, indices.data()));
      geom.commit();

      cpp::Material mat(rendererType, "ThinGlass");
      mat.setParam("attenuationDistance", 0.2f);
      mat.commit();

      cpp::GeometricModel model(geom);
      model.setParam("material", cpp::Data(mat));
      model.setParam("color",
                     cpp::Data(colors.size(), OSP_VEC4F, colors.data()));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Curves, curves);

  }  // namespace testing
}  // namespace ospray
