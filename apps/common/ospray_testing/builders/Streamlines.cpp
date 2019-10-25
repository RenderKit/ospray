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
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    struct Streamlines : public detail::Builder
    {
      Streamlines()           = default;
      ~Streamlines() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Streamlines::commit()
    {
      Builder::commit();

      addPlane = false;
    }

    cpp::Group Streamlines::buildGroup() const
    {
      cpp::Geometry slGeom("curves");

      std::vector<vec4f> points;
      std::vector<unsigned int> indices;
      std::vector<vec4f> colors;

      std::mt19937 rng(randomSeed);
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
          vec3f p, q;
          float startRadius, endRadius;

            p.x = radius * std::sin(d * M_PI / 180.f);
            p.y = h - 2;
            p.z = radius * std::cos(d * M_PI / 180.f);
            startRadius = 0.015f * std::sin(f * d * M_PI/180) + 0.02f;

            q.x = (radius - 0.05f) * std::sin((d+10) * M_PI / 180.f);
            q.y = h + hStep- 2;
            q.z = (radius - 0.05f) * std::cos((d+10) * M_PI / 180.f);
            endRadius =  0.015f * std::sin(f * (d+10) * M_PI/180) + 0.02f;
          if (d == dStart) {
            const vec3f rim = lerp(1.f + endRadius / length(q - p), q, p);
            const vec3f cap = lerp(1.f + startRadius/ length(rim - p), p, rim);
            points.push_back(vec4f(cap, 0.f));
            points.push_back(vec4f(rim, 0.f));
            points.push_back(vec4f(p, startRadius));
            points.push_back(vec4f(q, endRadius));
            indices.push_back(points.size() - 4);
            colors.push_back(c);
            colors.push_back(c);
          } else if (d +10 < dStart + dEnd && d + 20 > dStart + dEnd) {    
            const vec3f rim = lerp(1.f + startRadius / length(p - q), p, q);
            const vec3f cap = lerp(1.f + endRadius / length(rim - q), q, rim);
            points.push_back(vec4f(p, startRadius));
            points.push_back(vec4f(q, endRadius));
            points.push_back(vec4f(rim, 0.f));
            points.push_back(vec4f(cap, 0.f));
            indices.push_back(points.size() - 7);
            indices.push_back(points.size() - 6);
            indices.push_back(points.size() - 5);
            indices.push_back(points.size() - 4);
            colors.push_back(c);
            colors.push_back(c);
          } else if ((d != dStart && d != dStart + 10) && d + 20 < dStart + dEnd ){
            points.push_back(vec4f(p, startRadius));
            indices.push_back(points.size() - 4);
          }
          colors.push_back(c);
          radius -= 0.05f;
        }      
      }

      slGeom.setParam("vertex.position_radius", cpp::Data(points));
      slGeom.setParam("index", cpp::Data(indices));
      slGeom.setParam("vertex.color", cpp::Data(colors));
      slGeom.setParam("type", int(OSP_ROUND));
      slGeom.setParam("basis", int(OSP_CATMULL_ROM));
      
      slGeom.commit();

      cpp::Material slMat(rendererType, "OBJMaterial");
      slMat.commit();

      cpp::GeometricModel model(slGeom);
      model.setParam("material", cpp::Data(slMat));
      model.commit();

      cpp::Group group;

      group.setParam("geometry", cpp::Data(model));
      group.commit();

      return group;
    }

    OSP_REGISTER_TESTING_BUILDER(Streamlines, streamlines);

  }  // namespace testing
}  // namespace ospray
