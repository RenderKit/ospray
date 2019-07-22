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

#include <cmath>
#include <random>
#include <vector>

using namespace ospcommon;

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
      std::vector<vec4f> points;
      std::vector<int> indices;
      std::vector<vec4f> colors;

      std::random_device rd;
      std::default_random_engine rng(rd());
      std::uniform_real_distribution<float> radDist(0.5f, 1.5f);
      std::uniform_real_distribution<float> stepDist(0.0001f, 0.01f);
      std::uniform_int_distribution<int> dDist(0, 360);
      std::uniform_real_distribution<float> freqDist(0.5f, 1.5f);
      std::uniform_real_distribution<float> colorDist(0.1f, 1.f);

      int offset = 0;

      // create multiple lines
      int numLines = 50;
      for (int l = 0; l < numLines; l++) {
        int dStart   = dDist(rng);
        float radius = radDist(rng);
        float h = 0, hStep = stepDist(rng);
        float f = freqDist(rng);

        vec4f c(colorDist(rng), colorDist(rng), colorDist(rng), 1.f);

        for (int d = dStart; d < dStart + 360; d++, h += hStep) {
          vec4f p;
          p.x = radius * std::sin(d * M_PI / 180.f);
          p.y = h;
          p.z = radius * std::cos(d * M_PI / 180.f);
          p.w = 0.01f * std::sin(f * (d * M_PI / 180.f)) + 0.01f;
          points.push_back(p);
          colors.push_back(c);
          if (d < 359)
            indices.push_back(offset + d);
        }
        offset += 360;
      }

      OSPData pointsData  = ospNewData(points.size(), OSP_VEC4F, points.data());
      OSPData indicesData = ospNewData(indices.size(), OSP_INT, indices.data());
      OSPData colorsData  = ospNewData(colors.size(), OSP_VEC4F, colors.data());
      ospSetData(slGeom, "vertex.position", pointsData);
      ospSetData(slGeom, "index", indicesData);
      ospSetData(slGeom, "vertex.color", colorsData);
      ospCommit(slGeom);

      auto slModel = ospNewGeometricModel(slGeom);
      ospCommit(slModel);

      OSPGroup group = ospNewGroup();
      auto models    = ospNewData(1, OSP_OBJECT, &slModel);
      ospSetData(group, "geometry", models);
      ospCommit(group);
      ospRelease(models);

      box3f bounds = empty;

      OSPMaterial slMat = ospNewMaterial(renderer_type.c_str(), "OBJMaterial");
      ospCommit(slMat);

      OSPInstance instance = ospNewInstance(group);
      ospCommit(instance);

      OSPTestingGeometry retval;
      retval.geometry = slGeom;
      retval.model    = slModel;
      retval.group    = group;
      retval.instance = instance;
      retval.bounds   = reinterpret_cast<osp_box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_GEOMETRY(TestStreamlines, streamlines);

  }  // namespace testing
}  // namespace ospray
