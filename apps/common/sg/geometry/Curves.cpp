// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "Curves.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    Curves::Curves() : Geometry("curves")
    {
    }

    std::string Curves::toString() const
    {
      return "ospray::sg::Curves";
    }

    box3f Curves::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      auto vtx = child("vertex").nodeAs<DataBuffer>()->baseAs<vec4f>();
      auto idx = child("index").nodeAs<DataBuffer>();

      for (uint32_t e = 0; e < idx->size(); e++) {
        uint32_t i = idx->get<int>(e);
        for (uint32_t v = i; v < i + 4; v++) {
          float radius = vtx[v][3];
          vec3f vertex(vtx[v].x, vtx[v].y, vtx[v].z);
          bbox.extend(vertex - radius);
          bbox.extend(vertex + radius);
        }
      }

      child("bounds") = bbox;

      return bbox;
    }

    void Curves::preCommit(RenderContext &ctx)
    {
      if (!hasChild("vertex") || !hasChild("index"))
        throw std::runtime_error("#osp.sg - error, invalid Curves!");

      Geometry::preCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(Curves);

  } // ::ospray::sg
} // ::ospray
