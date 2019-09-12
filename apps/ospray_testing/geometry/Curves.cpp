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
#include "ospcommon/math/box.h"

#include <cmath>
#include <random>
#include <vector>

using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct TestCurves : public Geometry
    {
      TestCurves()           = default;
      ~TestCurves() override = default;

      void kleinBottle(std::vector<vec4f> &points,
                       std::vector<int> &indices,
                       std::vector<vec4f> &colors,
                       OSPGeometry &geom) const;
      void embreeTutorialCurve(std::vector<vec4f> &points,
                               std::vector<int> &indices,
                               std::vector<vec4f> &colors,
                               OSPGeometry &geom) const;
      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    void TestCurves::kleinBottle(std::vector<vec4f> &points,
                                 std::vector<int> &indices,
                                 std::vector<vec4f> &colors,
                                 OSPGeometry &geom) const
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

      ospSetInt(geom, "type", OSP_ROUND);
      ospSetInt(geom, "basis", OSP_BSPLINE);
    }

    // from Embree's curve_geometry tutorial
    void TestCurves::embreeTutorialCurve(std::vector<vec4f> &points,
                                         std::vector<int> &indices,
                                         std::vector<vec4f> &colors,
                                         OSPGeometry &geom) const
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

      ospSetInt(geom, "type", OSP_ROUND);
      ospSetInt(geom, "basis", OSP_BSPLINE);
    }

    OSPTestingGeometry TestCurves::createGeometry(
        const std::string &renderer_type) const
    {
      auto geom = ospNewGeometry("curves");
      std::vector<vec4f> points;
      std::vector<int> indices;
      std::vector<vec4f> colors;
      embreeTutorialCurve(points, indices, colors, geom);

      OSPData pointsData  = ospNewData(points.size(), OSP_VEC4F, points.data());
      OSPData indicesData =
          ospNewData(indices.size(), OSP_UINT, indices.data());
      OSPData colorsData  = ospNewData(colors.size(), OSP_VEC4F, colors.data());
      ospSetData(geom, "vertex.position", pointsData);
      ospSetData(geom, "index", indicesData);
      ospCommit(geom);

      OSPMaterial mat = ospNewMaterial(renderer_type.c_str(), "ThinGlass");
      ospSetFloat(mat, "attenuationDistance", 0.2f);
      ospCommit(mat);

      auto model = ospNewGeometricModel(geom);
      ospSetObject(model, "material", mat);
      ospSetObject(model, "prim.color", colorsData);
      ospCommit(model);
      ospRelease(mat);

      OSPGroup group = ospNewGroup();
      auto models = ospNewData(1, OSP_GEOMETRIC_MODEL, &model);
      ospSetData(group, "geometry", models);
      ospCommit(group);
      ospRelease(models);

      box3f bounds = empty;

      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.geometry = geom;
      retval.model    = model;
      retval.group    = group;
      retval.instance = instance;

      std::memcpy(&retval.bounds, &bounds, sizeof(bounds));

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(TestCurves, curves);

  }  // namespace testing
}  // namespace ospray
