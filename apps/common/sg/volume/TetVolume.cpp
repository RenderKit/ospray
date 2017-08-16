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

// sg
#include "TetVolume.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    std::string TetVolume::toString() const
    {
      return "ospray::sg::TetVolume";
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

      ospVolume = ospNewVolume("tetrahedral_volume");
      setValue((OSPObject)ospVolume);

      if (!hasChild("vertices"))
        throw std::runtime_error("#osp:sg TetVolume -> no 'vertices' array!");
      else if (!hasChild("tetrahedra"))
        throw std::runtime_error("#osp:sg TetVolume -> no 'tetrahedra' array!");
      else if (!hasChild("field"))
        throw std::runtime_error("#osp:sg TetVolume -> no 'field' array!");

      auto vertices   = child("vertices").nodeAs<DataBuffer>();
      auto tetrahedra = child("tetrahedra").nodeAs<DataBuffer>();
      auto field      = child("field").nodeAs<DataBuffer>();

      ospcommon::box3f bounds;
      for (int i = 0; i < vertices->size(); ++i)
        bounds.extend(vertices->get<vec3f>(i));
      child("bounds").setValue(bounds);

      auto *field_array = field->baseAs<float>();
      auto minMax = std::minmax_element(field_array,
                                        field_array + field->size() - 1);
      vec2f voxelRange(*minMax.first, *minMax.second);

      child("voxelRange").setValue(voxelRange);
      child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
      float iso = child("isosurface").valueAs<float>();
      if (iso < voxelRange.x || iso > voxelRange.y)
        child("isosurface").setValue((voxelRange.y-voxelRange.x)/2.f);
      child("transferFunction")["valueRange"].setValue(voxelRange);

      //NOTE: These children are removed because they are not yet implemnted
      //      in the base Tetrahedral volume in OSPRay!
      child("gradientShadingEnabled").setValue(false);
    }

    OSP_REGISTER_SG_NODE(TetVolume);

  } // ::ospray::sg
} // ::ospray
