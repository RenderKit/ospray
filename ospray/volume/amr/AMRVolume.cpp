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

#include "AMRVolume.h"
// ospray
#include "common/Model.h"
#include "common/Data.h"
#include "transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"
// ispc exports
#include "AMRVolume_ispc.h"
#include "method_finest_ispc.h"
#include "method_current_ispc.h"
#include "method_octant_ispc.h"
// stl
#include <set>
#include <map>

namespace ospray {

    AMRVolume::AMRVolume()
    {
      ispcEquivalent = ispc::AMRVolume_create(this);
    }

    std::string AMRVolume::toString() const
    {
      return "ospray::AMRVolume";
    }

    /*! Copy voxels into the volume at the given index (non-zero
      return value indicates success). */
    int AMRVolume::setRegion(const void *, const vec3i &, const vec3i &)
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
        setParam("voxelRange", voxelRange);
      else
        voxelRange = getParam2f("voxelRange", voxelRange);

      auto methodStringFromEnv =
          utility::getEnvVar<std::string>("OSPRAY_AMR_METHOD");

      std::string methodString =
          methodStringFromEnv.value_or(getParamString("amrMethod","current"));

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

      data  = make_unique<amr::AMRData>(*brickInfoData,*brickDataData);
      accel = make_unique<amr::AMRAccel>(*data);

      // finding coarset cell size + finest level cell width
      float coarsestCellWidth = 0.f;
      float finestLevelCellWidth = data->brick[0].cellWidth;
      box3i rootLevelBox = empty;

      for (auto &b : data->brick) {
        if (b.level == 0)
          rootLevelBox.extend(b.box);
        finestLevelCellWidth = min(finestLevelCellWidth, b.cellWidth);
        coarsestCellWidth    = max(coarsestCellWidth, b.cellWidth);
      }

      vec3i rootGridDims = rootLevelBox.size() + vec3i(1);
      ospLogF(1) << "found root level dimensions of " << rootGridDims;
      ospLogF(1) << "coarsest cell width is " << coarsestCellWidth << std::endl;

      auto rateFromEnv =
          utility::getEnvVar<float>("OSPRAY_AMR_SAMPLING_STEP");

      float samplingStep = rateFromEnv.value_or(0.1f * coarsestCellWidth);

      box3f worldBounds = accel->worldBounds;

      const vec3f gridSpacing = getParam3f("gridSpacing", vec3f(1.f));
      const vec3f gridOrigin  = getParam3f("gridOrigin", vec3f(0.f));

      voxelType =  getParamString("voxelType", "unspecified");
      auto voxelTypeID = getVoxelType();

      switch (voxelTypeID) {
      case OSP_UCHAR:
        break;
      case OSP_SHORT:
        break;
      case OSP_USHORT:
        break;
      case OSP_FLOAT:
        break;
      case OSP_DOUBLE:
        break;
      default:
        throw std::runtime_error("amrVolume unsupported voxel type '"
                                 + voxelType + "'");
      }

      ispc::AMRVolume_set(getIE(), (ispc::box3f&)worldBounds, samplingStep,
                          (const ispc::vec3f&)gridOrigin,
                          (const ispc::vec3f&)gridSpacing);

      ispc::AMRVolume_setAMR(getIE(),
                             accel->node.size(),
                             &accel->node[0],
                             accel->leaf.size(),
                             &accel->leaf[0],
                             accel->level.size(),
                             &accel->level[0],
                             voxelTypeID,
                             (ispc::box3f &)worldBounds);

      tasking::parallel_for(accel->leaf.size(),[&](size_t leafID) {
        ispc::AMRVolume_computeValueRangeOfLeaf(getIE(), leafID);
      });
    }

  OSPDataType AMRVolume::getVoxelType()
  {
    return (voxelType == "") ? typeForString(getParamString("voxelType", "unspecified")):
                      typeForString(voxelType.c_str());
  }

    OSP_REGISTER_VOLUME(AMRVolume, AMRVolume);
    OSP_REGISTER_VOLUME(AMRVolume, amr_volume);

} // ::ospray
