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
#include <vector>
// ospcommon
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"
using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct SimpleStructuredVolume : public Volume
    {
      SimpleStructuredVolume()           = default;
      ~SimpleStructuredVolume() override = default;

      OSPTestingVolume createVolume() const override;

     private:
      vec3l dims{100, 100, 100};
      vec3f spacing{1.f};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingVolume SimpleStructuredVolume::createVolume() const
    {
      OSPVolume volume = ospNewVolume("shared_structured_volume");

      // generate volume data

      auto numVoxels = dims.product();

      std::vector<float> voxels(numVoxels);

      const float r = 0.03f * reduce_max(dims);
      const float R = 0.08f * reduce_max(dims);

      tasking::parallel_for(dims.z, [&](int64_t z) {
        for (int y = 0; y < dims.y; ++y) {
          for (int x = 0; x < dims.x; ++x) {
            const float X = x - dims.x / 2;
            const float Y = y - dims.y / 2;
            const float Z = z - dims.z / 2;

            float d = (R - std::sqrt(X * X + Y * Y));
            voxels[dims.x * dims.y * z + dims.x * y + x] =
                r * r - d * d - Z * Z;
          }
        }
      });

      // calculate voxel range

      range1f voxelRange;
      std::for_each(voxels.begin(), voxels.end(), [&](float &v) {
        if (!std::isnan(v))
          voxelRange.extend(v);
      });

      // create OSPRay objects and return results

      OSPData voxelData = ospNewData(numVoxels, OSP_FLOAT, voxels.data());
      ospSetObject(volume, "voxelData", voxelData);
      ospRelease(voxelData);

      ospSetString(volume, "voxelType", "float");
      ospSet3i(volume, "dimensions", dims.x, dims.y, dims.z);

      const auto range  = voxelRange.toVec2f();
      const auto bounds = box3f(vec3f(0.f), dims * spacing);

      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = reinterpret_cast<const osp::vec2f &>(range);
      retval.bounds     = reinterpret_cast<const osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(SimpleStructuredVolume, structured_volume);
    OSP_REGISTER_TESTING_VOLUME(SimpleStructuredVolume,
                                simple_structured_volume);

  }  // namespace testing
}  // namespace ospray
