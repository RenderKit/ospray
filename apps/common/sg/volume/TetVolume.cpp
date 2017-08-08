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

#pragma once

// sg
#include "TetVolume.h"

namespace ospray {
  namespace sg {

    std::string TetVolume::toString() const
    {
      return "ospray::sg::TetVolume";
    }

    box3f TetVolume::bounds() const
    {
      return {};
    }

    void TetVolume::preCommit(RenderContext &ctx)
    {
      auto ospVolume = (OSPVolume)valueAs<OSPObject>();
      if (ospVolume) {
        ospCommit(ospVolume);
        if (child("isosurfaceEnabled").valueAs<bool>() == true
            && isosurfacesGeometry) {
          OSPData isovaluesData = ospNewData(1, OSP_FLOAT,
            &child("isosurface").valueAs<float>());
          ospSetData(isosurfacesGeometry, "isovalues", isovaluesData);
          ospCommit(isosurfacesGeometry);
        }
        return;
      }

#if 0
      child("voxelRange").setValue(voxelRange);
      child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
      float iso = child("isosurface").valueAs<float>();
      if (iso < voxelRange.x || iso > voxelRange.y)
        child("isosurface").setValue((voxelRange.y-voxelRange.x)/2.f);
      child("transferFunction")["valueRange"].setValue(voxelRange);
#endif
    }

    void TetVolume::postCommit(RenderContext &ctx)
    {
      auto ospVolume = (OSPVolume)valueAs<OSPObject>();
      ospSetObject(ospVolume, "transferFunction",
                   child("transferFunction").valueAs<OSPObject>());
      ospCommit(ospVolume);
    }

    OSP_REGISTER_SG_NODE(TetVolume);

  } // ::ospray::sg
} // ::ospray
