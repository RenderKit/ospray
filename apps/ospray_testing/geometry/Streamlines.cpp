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

    struct TestStreamlines : public Geometry
    {
      TestStreamlines()           = default;
      ~TestStreamlines() override = default;

      OSPTestingGeometry createGeometry(
          const std::string &renderer_type) const override;
    };

    OSPTestingGeometry TestStreamlines::createGeometry(
        const std::string &renderer_type) const
    {
      auto slGeom = ospNewGeometry("streamlines");
      std::vector<vec3f> points;
      std::vector<float> radii;
      std::vector<int> indices;
      std::vector<vec4f> colors;

      std::random_device rd;
      std::default_random_engine rng(rd());
      std::uniform_real_distribution<float> radDist(0.5f, 1.5f);
      std::uniform_real_distribution<float> stepDist(0.001f, 0.1f);
      std::uniform_int_distribution<int> sDist(0, 360);
      std::uniform_int_distribution<int> dDist(360, 720);
      std::uniform_real_distribution<float> freqDist(0.5f, 1.5f);

      // create multiple lines
      int numLines = 100;
      for (int l = 0; l < numLines; l++) {
        int dStart   = sDist(rng);
        int dEnd     = dDist(rng);
        float radius = radDist(rng);
        float h      = 0;
        float hStep  = stepDist(rng);
        float f      = freqDist(rng);

        float r = (720 - dEnd) / 360.f;
        vec4f c(r, 1 - r, 1 - r / 2, 1.f);

        // spiral up with changing radius of curvature
        for (int d = dStart; d < dStart + dEnd; d += 10, h += hStep) {
          if (d != dStart)
            indices.push_back(points.size() - 1);

          vec3f p;
          p.x = radius * std::sin(d * M_PI / 180.f);
          p.y = h - 2;
          p.z = radius * std::cos(d * M_PI / 180.f);

          points.push_back(p);
          radii.push_back(0.015f * std::sin(f * (d * M_PI / 180.f)) + 0.01f);
          colors.push_back(c);

          radius -= 0.05f;
        }
      }

      OSPData pointsData = ospNewData(points.size(), OSP_VEC3F, points.data());
      OSPData radiusData = ospNewData(radii.size(), OSP_FLOAT, radii.data());
      OSPData indicesData =
          ospNewData(indices.size(), OSP_UINT, indices.data());
      OSPData colorsData  = ospNewData(colors.size(), OSP_VEC4F, colors.data());
      ospSetData(slGeom, "vertex.position", pointsData);
      ospSetData(slGeom, "vertex.radius", radiusData);
      ospSetData(slGeom, "index", indicesData);
      ospSetData(slGeom, "vertex.color", colorsData);
      ospCommit(slGeom);

      ospRelease(pointsData);
      ospRelease(indicesData);
      ospRelease(colorsData);

      auto slModel = ospNewGeometricModel(slGeom);
      ospCommit(slModel);

      OSPGroup group = ospNewGroup();
      auto models = ospNewData(1, OSP_GEOMETRIC_MODEL, &slModel);
      ospSetData(group, "geometry", models);
      ospCommit(group);
      ospRelease(models);

      box3f bounds = empty;

      OSPMaterial slMat = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      ospCommit(slMat);

      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.auxData = radiusData;
      retval.geometry = slGeom;
      retval.model    = slModel;
      retval.group    = group;
      retval.instance = instance;

      std::memcpy(&retval.bounds, &bounds, sizeof(bounds));

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(TestStreamlines, streamlines);

  }  // namespace testing
}  // namespace ospray
