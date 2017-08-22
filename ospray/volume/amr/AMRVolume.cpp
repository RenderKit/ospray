// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "AMRVolume.h"
// ospray
#include "ospray/common/Model.h"
#include "ospray/common/Data.h"
#include "ospray/transferFunction/TransferFunction.h"
#include "ospcommon/tasking/parallel_for.h"
// ispc exports
#include "AMRVolume_ispc.h"
#include "finest_ispc.h"
#include "current_ispc.h"
#include "octant_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {
  namespace amr {

    AMRVolume::AMRVolume()
    {
      ispcEquivalent = ispc::AMRVolume_create(this);
    }

    /*! Copy voxels into the volume at the given index (non-zero
      return value indicates success). */
    int AMRVolume::setRegion(const void *source,
                             const vec3i &index,
                             const vec3i &count)
    {
      FATAL("'setRegion()' doesn't make sense for AMR volumes; "
            "they can only be set from existing data");
    }

    //! Allocate storage and populate the volume.
    void AMRVolume::commit()
    {
      updateEditableParameters();

      // Make the voxel value range visible to the application.
      if (findParam("voxelRange") == nullptr)
        set("voxelRange", voxelRange);
      else
        voxelRange = getParam2f("voxelRange", voxelRange);

      auto methodStringFromEnv = getEnvVar<std::string>("OSPRAY_AMR_METHOD");
      std::string methodString = "current";

      if (methodStringFromEnv.first)
        methodString = methodStringFromEnv.second;
      else
        methodString = getParamString("amrMethod","current");

      if (methodString == "finest" || methodString == "finestLevel")
        ispc::AMR_install_finest(getIE());
      else if (methodString == "current" || methodString == "currentLevel")
        ispc::AMR_install_current(getIE());
      else if (methodString == "octant")
        ispc::AMR_install_octant(getIE());

      if (data != nullptr) //TODO: support data updates
        return;

      brickInfoData = getParamData("brickInfo");
      assert(brickInfoData);
      assert(brickInfoData->data);

      brickDataData = getParamData("brickData");
      assert(brickDataData);
      assert(brickDataData->data);

      assert(data == nullptr);
      data = new AMRData(*brickInfoData,*brickDataData);
      assert(accel == nullptr);
      accel = new AMRAccel(*data);

      Ref<TransferFunction> xf = (TransferFunction*)getParamObject("transferFunction");
      assert(xf);

      float finestLevelCellWidth = data->brick[0]->cellWidth;
      box3i rootLevelBox = empty;
      for (int i=0;i<data->numBricks;i++) {
        if (data->brick[i]->level == 0)
          rootLevelBox.extend(data->brick[i]->box);
        finestLevelCellWidth = min(finestLevelCellWidth,data->brick[i]->cellWidth);
      }
      vec3i rootGridDims = rootLevelBox.size()+vec3i(1);
      ospLogF(1) << "found root level dimensions of " << rootGridDims;

      // finding coarset cell size:
      float coarsestCellWidth = 0.f;
      for (int i=0;i<data->numBricks;i++)
        coarsestCellWidth = max(coarsestCellWidth,data->brick[i]->cellWidth);
      ospLogF(1) << "coarsest cell width is " << coarsestCellWidth << std::endl;
      float samplingStep = 0.1f*coarsestCellWidth;

      auto rateFromString = getEnvVar<std::string>("OSPRAY_AMR_SAMPLING_STEP");
      if (rateFromString.first)
        samplingStep = atof(rateFromString.second.c_str());

      box3f worldBounds = accel->worldBounds;

      ispc::AMRVolume_set(getIE(),
                          xf->getIE(),
                          (ispc::box3f&)worldBounds,
                          samplingStep);

      ispc::AMRVolume_setAMR(getIE(),
                             accel->node.size(),
                             &accel->node[0],
                             accel->leaf.size(),
                             &accel->leaf[0],
                             accel->level.size(),
                             &accel->level[0],
                             (ispc::box3f &)worldBounds);

      ospcommon::tasking::parallel_for(accel->leaf.size(),[&](int leafID) {
        ispc::AMRVolume_computeValueRangeOfLeaf(getIE(), leafID);
      });
    }

    //! The c++-side, scalar sampler we're calling back to
    extern "C"
    void ospray_amr_sample(VolumeSampler *cppSampler,
                           float &result,
                           const vec3f &pos)
    {
      throw std::runtime_error("do not do this any more ...\n");
      result = cppSampler->sample(pos);
    }

        //! The c++-side, scalar sampler we're calling back to
    extern "C"
    void ospray_amr_sampleLevel(VolumeSampler *cppSampler,
                                float &result,
                                const vec3f &pos, float& width)
    {
      throw std::runtime_error("do not do this any more ...\n");
      result = cppSampler->sampleLevel(pos, width);
    }

    //! The c++-side, scalar gradient sampler we're calling back to
    extern "C"
    void ospray_amr_gradient(VolumeSampler *cppSampler,
                             vec3f &result,
                             const vec3f &pos)
    {
      throw std::runtime_error("do not do this any more ...\n");
      result = cppSampler->gradient(pos);
    }

    OSP_REGISTER_VOLUME(AMRVolume,AMRVolume);
    OSP_REGISTER_VOLUME(AMRVolume,amr_volume);
  }

} // ::ospray