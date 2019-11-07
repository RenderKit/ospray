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

     private:
      bool smooth{false};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void Streamlines::commit()
    {
      Builder::commit();

      smooth = getParam<bool>("smooth", false);

      addPlane = false;
    }

    cpp::Group Streamlines::buildGroup() const
    {
      cpp::Geometry slGeom("streamlines");

      std::vector<vec3f> points;
      std::vector<float> radii;
      std::vector<unsigned int> indices;
      std::vector<vec4f> colors;

      std::default_random_engine rng(randomSeed);
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

      slGeom.setParam("vertex.position", cpp::Data(points));
      slGeom.setParam("vertex.radius", cpp::Data(radii));
      slGeom.setParam("index", cpp::Data(indices));
      slGeom.setParam("vertex.color", cpp::Data(colors));
      slGeom.setParam("smooth", smooth);
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
