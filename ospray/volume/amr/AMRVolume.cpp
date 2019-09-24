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
#include "common/Data.h"
#include "volume/transferFunction/TransferFunction.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"
// ispc exports
#include "AMRVolume_ispc.h"
#include "method_current_ispc.h"
#include "method_finest_ispc.h"
#include "method_octant_ispc.h"
// stl
#include <map>
#include <set>

namespace ospray {

  AMRVolume::AMRVolume()
  {
    ispcEquivalent = ispc::AMRVolume_create(this);
  }

  std::string AMRVolume::toString() const
  {
    return "ospray::AMRVolume";
  }

  //! Allocate storage and populate the volume.
  void AMRVolume::commit()
  {
    Volume::commit();

    voxelRange = getParam<vec2f>("voxelRange", vec2f(FLT_MAX, -FLT_MAX));

    amrMethod = getParam<OSPAMRMethod>("method", OSP_AMR_CURRENT);

    if(amrMethod == OSP_AMR_CURRENT)
      ispc::AMR_install_current(getIE());
    else if(amrMethod == OSP_AMR_FINEST)
      ispc::AMR_install_finest(getIE());
    else if(amrMethod == OSP_AMR_OCTANT)
      ispc::AMR_install_octant(getIE());

    if (data != nullptr)  // TODO: support data updates
      return;

    blockBoundsData = getParamData("block.bounds");
    if (blockBoundsData.ptr == nullptr)
      throw std::runtime_error("amr volume must have 'block.bounds' array");

    refinementLevelsData = getParamData("block.level");
    if (refinementLevelsData.ptr == nullptr)
      throw std::runtime_error("amr volume must have 'block.level' array");

    cellWidthsData = getParamData("block.cellWidth");
    if (cellWidthsData.ptr == nullptr)
      throw std::runtime_error("amr volume must have 'block.cellWidth' array");

    blockDataData = getParamData("block.data");
    if (blockDataData.ptr == nullptr)
      throw std::runtime_error("amr volume must have 'block.data' array");

    data  = make_unique<amr::AMRData>(*blockBoundsData,
                                     *refinementLevelsData,
                                     *cellWidthsData,
                                     *blockDataData);
    accel = make_unique<amr::AMRAccel>(*data);

    float coarsestCellWidth = *std::max_element(
        cellWidthsData->as<float>().begin(), cellWidthsData->as<float>().end());

    float samplingStep = 0.1f * coarsestCellWidth;

    bounds = accel->worldBounds;

    const vec3f gridSpacing = getParam<vec3f>("gridSpacing", vec3f(1.f));
    const vec3f gridOrigin  = getParam<vec3f>("gridOrigin", vec3f(0.f));

    voxelType = (OSPDataType)getParam<int>("voxelType", OSP_UNKNOWN);

    switch (voxelType) {
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
      throw std::runtime_error("amr volume 'voxelType' is invalid type " +
                               stringFor(voxelType) +
                               ". Must be one of: OSP_UCHAR, OSP_SHORT, "
                               "OSP_USHORT, OSP_FLOAT, OSP_DOUBLE");
    }

    ispc::AMRVolume_set(getIE(),
                        (ispc::box3f &)bounds,
                        samplingStep,
                        (const ispc::vec3f &)gridOrigin,
                        (const ispc::vec3f &)gridSpacing);

    ispc::AMRVolume_setAMR(getIE(),
                           accel->node.size(),
                           &accel->node[0],
                           accel->leaf.size(),
                           &accel->leaf[0],
                           accel->level.size(),
                           &accel->level[0],
                           voxelType,
                           (ispc::box3f &)bounds);

    tasking::parallel_for(accel->leaf.size(), [&](size_t leafID) {
      ispc::AMRVolume_computeValueRangeOfLeaf(getIE(), leafID);
    });
  }

  OSP_REGISTER_VOLUME(AMRVolume, AMRVolume);
  OSP_REGISTER_VOLUME(AMRVolume, amr_volume);

}  // namespace ospray
