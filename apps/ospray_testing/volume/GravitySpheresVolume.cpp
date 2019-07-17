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
// apps/utility
#include "../../utility/rawToAMR.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct GravitySpheresVolume : public Volume
    {
      GravitySpheresVolume()           = default;
      ~GravitySpheresVolume() override = default;

      std::vector<float> generateVoxels(const size_t numPoints=10) const;
      OSPTestingVolume createVolume() const override;

     protected:
      size_t volumeDimension{256};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    std::vector<float> GravitySpheresVolume::generateVoxels(const size_t numPoints) const
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
      auto logicalToWorldCoordinates = [&](size_t i, size_t j, size_t k) {
        return vec3f(-1.f + float(i) / float(volumeDimension - 1) * 2.f,
                     -1.f + float(j) / float(volumeDimension - 1) * 2.f,
                     -1.f + float(k) / float(volumeDimension - 1) * 2.f);
      };

      // generate voxels
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

      return voxels;
    }

    OSPTestingVolume GravitySpheresVolume::createVolume() const
    {
      // create a structured volume and assign attributes
      OSPVolume volume = ospNewVolume("block_bricked_volume");

      ospSetVec3i(volume,
               "dimensions",
               volumeDimension,
               volumeDimension,
               volumeDimension);
      ospSetString(volume, "voxelType", "float");
      ospSetVec3f(volume, "gridOrigin", -1.f, -1.f, -1.f);

      const float gridSpacing = 2.f / float(volumeDimension);
      ospSetVec3f(volume, "gridSpacing", gridSpacing, gridSpacing, gridSpacing);

      // generate volume values
      std::vector<float> voxels = generateVoxels();

      vec3i regionStart{0, 0, 0};
      vec3i regionEnd{
          int(volumeDimension), int(volumeDimension), int(volumeDimension)};

      // set the volume data
      ospSetRegion(volume, voxels.data(), &regionStart.x, &regionEnd.x);

      // create OSPRay objects and return results

      const auto range = vec2f(0.f, 10.f);
      const auto bounds =
          box3f(vec3f(-1.f), vec3f(-1.f) + volumeDimension * gridSpacing);

      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = reinterpret_cast<const osp_vec2f &>(range);
      retval.bounds     = reinterpret_cast<const osp_box3f &>(bounds);

      ospCommit(volume);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(GravitySpheresVolume, gravity_spheres_volume);

    struct GravitySpheresAMRVolume : public GravitySpheresVolume
    {
      GravitySpheresAMRVolume()           = default;
      ~GravitySpheresAMRVolume() override = default;

      OSPTestingVolume createVolume() const override;
    };

    OSPTestingVolume GravitySpheresAMRVolume::createVolume() const
    {
      // generate the structured volume voxel data
      std::vector<float> voxels = generateVoxels(20);

      // initialize constants for converting to AMR
      vec3f volumeDims(volumeDimension, volumeDimension, volumeDimension);
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
                           volumeDims,
                           numLevels,
                           blockSize,
                           refinementLevel,
                           threshold,
                           blockBounds,
                           refinementLevels,
                           cellWidths,
                           blockDataVectors);

      for (const std::vector<float> &bd : blockDataVectors) {
        OSPData data =
            ospNewData(bd.size(), OSP_FLOAT, bd.data(), OSP_DATA_SHARED_BUFFER);
        blockData.push_back(data);
      }

      OSPData blockDataData = ospNewData(
          blockData.size(), OSP_DATA, blockData.data(), OSP_DATA_SHARED_BUFFER);
      ospCommit(blockDataData);

      OSPData blockBoundsData = ospNewData(blockBounds.size(),
                                           OSP_BOX3I,
                                           blockBounds.data(),
                                           OSP_DATA_SHARED_BUFFER);
      ospCommit(blockBoundsData);

      OSPData refinementLevelsData = ospNewData(refinementLevels.size(),
                                                OSP_INT,
                                                refinementLevels.data(),
                                                OSP_DATA_SHARED_BUFFER);
      ospCommit(refinementLevelsData);

      OSPData cellWidthsData = ospNewData(cellWidths.size(),
                                          OSP_FLOAT,
                                          cellWidths.data(),
                                          OSP_DATA_SHARED_BUFFER);
      ospCommit(cellWidthsData);

      // create an AMR volume and assign attributes
      OSPVolume volume = ospNewVolume("amr_volume");

      ospSetString(volume, "voxelType", "float");
      ospSetData(volume, "block.data", blockDataData);
      ospSetData(volume, "block.bounds", blockBoundsData);
      ospSetData(volume, "block.level", refinementLevelsData);
      ospSetData(volume, "block.cellWidth", cellWidthsData);

      ospCommit(volume);

      float ub = std::pow((float)refinementLevel, (float)numLevels + 1);

      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = osp_vec2f{0, 100};
      retval.bounds     = osp_box3f{osp_vec3f{0, 0, 0}, osp_vec3f{ub, ub, ub}};

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(GravitySpheresAMRVolume,
                                gravity_spheres_amr_volume);

  }  // namespace testing
}  // namespace ospray
