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
#include "ospcommon/tasking/parallel_for.h"
// raw_to_amr
#include "rawToAMR.h"

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
  namespace testing {

    using VoxelArray = std::vector<float>;

    struct GravitySpheres : public detail::Builder
    {
      GravitySpheres(bool addVolume     = true,
                     bool asAMR         = false,
                     bool addIsosurface = false);
      ~GravitySpheres() override = default;

      void commit() override;

      cpp::Group buildGroup() const override;

     private:
      VoxelArray generateVoxels() const;

      cpp::Volume createStructuredVolume(const VoxelArray &voxels) const;
      cpp::Volume createAMRVolume(const VoxelArray &voxels) const;

      // Data //

      vec3i volumeDimensions{128};
      int numPoints{10};
      bool withVolume{true};
      bool createAsAMR{false};
      bool withIsosurface{false};
      float isovalue{2.5f};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    GravitySpheres::GravitySpheres(bool addVolume,
                                   bool asAMR,
                                   bool addIsosurface)
        : withVolume(addVolume),
          createAsAMR(asAMR),
          withIsosurface(addIsosurface)
    {
    }

    void GravitySpheres::commit()
    {
      Builder::commit();

      volumeDimensions = getParam<vec3i>("volumeDimensions", vec3i(128));
      numPoints        = getParam<int>("numPoints", 10);
      withVolume       = getParam<bool>("withVolume", withVolume);
      createAsAMR      = getParam<bool>("asAMR", createAsAMR);
      withIsosurface   = getParam<bool>("withIsosurface", withIsosurface);
      isovalue         = getParam<float>("isovalue", 2.5f);

      addPlane = false;
    }

    cpp::Group GravitySpheres::buildGroup() const
    {
      auto voxels     = generateVoxels();
      auto voxelRange = vec2f(0.f, 10.f);

      cpp::Volume volume = createAsAMR ? createAMRVolume(voxels)
                                       : createStructuredVolume(voxels);

      cpp::VolumetricModel model(volume);
      model.setParam("transferFunction", makeTransferFunction(voxelRange));
      model.commit();

      cpp::Group group;

      if (withVolume)
        group.setParam("volume", cpp::Data(model));

      if (withIsosurface) {
        cpp::Geometry isoGeom("isosurfaces");
        isoGeom.setParam("isovalue", cpp::Data(isovalue));
        isoGeom.setParam("volume", model);
        isoGeom.commit();

        cpp::Material mat(rendererType, "OBJMaterial");
        mat.setParam("Ks", vec3f(0.2f));
        mat.commit();

        cpp::GeometricModel isoModel(isoGeom);
        isoModel.setParam("material", cpp::Data(mat));
        isoModel.commit();

        group.setParam("geometry", cpp::Data(isoModel));
      }

      group.commit();

      return group;
    }

    std::vector<float> GravitySpheres::generateVoxels() const
    {
      struct Point
      {
        vec3f center;
        float weight;
      };

      // create random number distributions for point center and weight
      std::mt19937 gen(randomSeed);

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
            size_t index = size_t(k) * volumeDimensions.z * volumeDimensions.y +
                           size_t(j) * volumeDimensions.x + size_t(i);

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
                      cpp::Data(volumeDimensions, {0}, voxels.data()));

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
      std::vector<cpp::Data> blockData;

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

      for (const std::vector<float> &bd : blockDataVectors)
        blockData.emplace_back(bd.size(), OSP_FLOAT, bd.data());

      // create an AMR volume and assign attributes
      cpp::Volume volume("amr_volume");

      volume.setParam("voxelType", int(OSP_FLOAT));
      volume.setParam("block.data", cpp::Data(blockData));
      volume.setParam("block.bounds", cpp::Data(blockBounds));
      volume.setParam("block.level", cpp::Data(refinementLevels));
      volume.setParam("block.cellWidth", cpp::Data(cellWidths));

      volume.commit();

      return volume;
    }

    OSP_REGISTER_TESTING_BUILDER(GravitySpheres, gravity_spheres_volume);

    OSP_REGISTER_TESTING_BUILDER(GravitySpheres(true, true, false),
                                 gravity_spheres_amr);

    OSP_REGISTER_TESTING_BUILDER(GravitySpheres(false, false, true),
                                 gravity_spheres_isosurface);

  }  // namespace testing
}  // namespace ospray
