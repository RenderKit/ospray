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
#include <vector>
// ospcommon
#include "ospcommon/math/box.h"
#include "ospcommon/tasking/parallel_for.h"
// apps/utility
#include "../../utility/rawToAMR.h"

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    using VoxelArray = std::vector<float>;

    struct GravitySpheres : public detail::Builder
    {
      GravitySpheres()           = default;
      ~GravitySpheres() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;
      cpp::World buildWorld() const override;

     private:
      VoxelArray generateVoxels() const;

      cpp::Volume createStructuredVolume(const VoxelArray &voxels) const;
      cpp::Volume createAMRVolume(const VoxelArray &voxels) const;

      // Data //

      vec3i volumeDimensions{128};
      int numPoints{10};
      bool createAsAMR{false};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    void GravitySpheres::commit()
    {
      Builder::commit();

      volumeDimensions = getParam<vec3i>("volumeDimensions", vec3i(128));
      numPoints        = getParam<int>("numPoints", 10);
      createAsAMR      = getParam<bool>("asAMR", false);
    }

    cpp::Group GravitySpheres::buildGroup() const
    {
      auto voxels     = generateVoxels();
      auto voxelRange = vec2f(0.f, 10.f);

      cpp::Volume volume = createAsAMR ? createAMRVolume(voxels)
                                       : createStructuredVolume(voxels);

      cpp::VolumetricModel model(volume);
      cpp::TransferFunction tf =
          ospTestingNewTransferFunction(voxelRange, "jet");
      tf.commit();
      model.setParam("transferFunction", tf);
      model.commit();

      cpp::Group group;

      group.setParam("volume", cpp::Data(1, OSP_VOLUMETRIC_MODEL, &model));
      group.commit();

      return group;
    }

    cpp::World GravitySpheres::buildWorld() const
    {
      cpp::World world;

      auto group = buildGroup();

      cpp::Instance inst(group);
      inst.commit();

      world.setParam("instance", cpp::Data(1, OSP_INSTANCE, &inst));
      world.commit();

      return world;
    }

    std::vector<float> GravitySpheres::generateVoxels() const
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

      // get world coordinate in [-1.f, 1.f] from logical coordinates in [0,
      // volumeDimension)
      auto logicalToWorldCoordinates = [&](int i, int j, int k) {
        return vec3f(-1.f + float(i) / float(volumeDimensions.x - 1) * 2.f,
                     -1.f + float(j) / float(volumeDimensions.y - 1) * 2.f,
                     -1.f + float(k) / float(volumeDimensions.z - 1) * 2.f);
      };

      // generate voxels
      std::vector<float> voxels(volumeDimensions.long_product());

      tasking::parallel_for(volumeDimensions.z, [&](int k) {
        for (int j = 0; j < volumeDimensions.y; j++) {
          for (int i = 0; i < volumeDimensions.x; i++) {
            // index in array
            size_t index = k * volumeDimensions.z * volumeDimensions.y +
                           j * volumeDimensions.x + i;

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

      return voxels;
    }

    cpp::Volume GravitySpheres::createStructuredVolume(
        const VoxelArray &voxels) const
    {
      cpp::Volume volume("structured_volume");

      volume.setParam("dimensions", volumeDimensions);
      volume.setParam("voxelType", int(OSP_FLOAT));
      volume.setParam("gridOrigin", vec3f(-1.f, -1.f, -1.f));
      volume.setParam("gridSpacing", vec3f(2.f / reduce_max(volumeDimensions)));
      volume.setParam("voxelData",
                      cpp::Data(voxels.size(), OSP_FLOAT, voxels.data()));

      volume.commit();
      return volume;
    }

    cpp::Volume GravitySpheres::createAMRVolume(const VoxelArray &voxels) const
    {
      const int numLevels       = 2;
      const int blockSize       = 16;
      const int refinementLevel = 4;
      const float threshold     = 1.0;

      std::vector<box3i> blockBounds;
      std::vector<int> refinementLevels;
      std::vector<float> cellWidths;
      std::vector<std::vector<float>> blockDataVectors;
      std::vector<OSPData> blockData;

      // convert the structured volume to AMR
      ospray::amr::makeAMR(voxels,
                           volumeDimensions,
                           numLevels,
                           blockSize,
                           refinementLevel,
                           threshold,
                           blockBounds,
                           refinementLevels,
                           cellWidths,
                           blockDataVectors);

      for (const std::vector<float> &bd : blockDataVectors) {
        OSPData data = ospNewData(bd.size(), OSP_FLOAT, bd.data());
        blockData.push_back(data);
      }

      OSPData blockDataData =
          ospNewData(blockData.size(), OSP_DATA, blockData.data());
      ospCommit(blockDataData);

      OSPData blockBoundsData =
          ospNewData(blockBounds.size(), OSP_BOX3I, blockBounds.data());
      ospCommit(blockBoundsData);

      OSPData refinementLevelsData =
          ospNewData(refinementLevels.size(), OSP_INT, refinementLevels.data());
      ospCommit(refinementLevelsData);

      OSPData cellWidthsData =
          ospNewData(cellWidths.size(), OSP_FLOAT, cellWidths.data());
      ospCommit(cellWidthsData);

      // create an AMR volume and assign attributes
      OSPVolume volume = ospNewVolume("amr_volume");

      ospSetInt(volume, "voxelType", OSP_FLOAT);
      ospSetObject(volume, "block.data", blockDataData);
      ospSetObject(volume, "block.bounds", blockBoundsData);
      ospSetObject(volume, "block.level", refinementLevelsData);
      ospSetObject(volume, "block.cellWidth", cellWidthsData);

      ospCommit(volume);

      return volume;
    }

    OSP_REGISTER_TESTING_BUILDER(GravitySpheres, gravity_spheres_volume);

  }  // namespace testing
}  // namespace ospray
