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

#include "Volume.h"
// stl
#include <random>
#include <vector>
// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct GravitySpheresVolume : public Volume
    {
      GravitySpheresVolume()           = default;
      ~GravitySpheresVolume() override = default;

      OSPTestingVolume createVolume() const override;

     private:
      size_t volumeDimension{256};
      size_t numPoints{10};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingVolume GravitySpheresVolume::createVolume() const
    {
      struct Point
      {
        vec3f center;
        float weight;
      };

      // create random number distributions for point center and weight
      std::random_device rd;
      std::mt19937 gen(rd());

      std::uniform_real_distribution<float> centerDistribution(-1.f, 1.f);
      std::uniform_real_distribution<float> weightDistribution(0.1f, 0.3f);

      // populate the points
      std::vector<Point> points(numPoints);

      for (auto &p : points) {
        p.center.x = centerDistribution(gen);
        p.center.y = centerDistribution(gen);
        p.center.z = centerDistribution(gen);

        p.weight = weightDistribution(gen);
      }

      // create a structured volume and assign attributes
      OSPVolume volume = ospNewVolume("block_bricked_volume");

      ospSet3i(volume,
               "dimensions",
               volumeDimension,
               volumeDimension,
               volumeDimension);
      ospSetString(volume, "voxelType", "float");
      ospSet3f(volume, "gridOrigin", -1.f, -1.f, -1.f);

      const float gridSpacing = 2.f / float(volumeDimension);
      ospSet3f(volume, "gridSpacing", gridSpacing, gridSpacing, gridSpacing);

      // get world coordinate in [-1.f, 1.f] from logical coordinates in [0,
      // volumeDimension)
      auto logicalToWorldCoordinates = [&](size_t i, size_t j, size_t k) {
        return vec3f(-1.f + float(i) / float(volumeDimension - 1) * 2.f,
                     -1.f + float(j) / float(volumeDimension - 1) * 2.f,
                     -1.f + float(k) / float(volumeDimension - 1) * 2.f);
      };

      // generate volume values
      std::vector<float> voxels(volumeDimension * volumeDimension *
                                volumeDimension);

      tasking::parallel_for(volumeDimension, [&](size_t k) {
        for (size_t j = 0; j < volumeDimension; j++) {
          for (size_t i = 0; i < volumeDimension; i++) {
            // index in array
            size_t index =
                k * volumeDimension * volumeDimension + j * volumeDimension + i;

            // compute volume value
            float value = 0.f;

            for (auto &p : points) {
              vec3f pointCoordinate = logicalToWorldCoordinates(i, j, k);
              const float distance  = length(pointCoordinate - p.center);

              // contribution proportional to weighted inverse-square distance
              // (i.e. gravity)
              value += p.weight / (distance * distance);
            }

            voxels[index] = value;
          }
        }
      });

      // set the volume data
      ospSetRegion(volume,
                   voxels.data(),
                   osp::vec3i{0, 0, 0},
                   osp::vec3i{int(volumeDimension),
                              int(volumeDimension),
                              int(volumeDimension)});

      // create OSPRay objects and return results

      const auto range = vec2f(0.f, 10.f);
      const auto bounds =
          box3f(vec3f(-1.f), vec3f(-1.f) + volumeDimension * gridSpacing);

      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = reinterpret_cast<const osp::vec2f &>(range);
      retval.bounds     = reinterpret_cast<const osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(GravitySpheresVolume, gravity_spheres_volume);

  }  // namespace testing
}  // namespace ospray
