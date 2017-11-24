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

#include "sg/geometry/TriangleMesh.h"
#include "sg/common/Data.h"

namespace ospray {
  namespace sg {

    TriangleMesh::TriangleMesh() : Geometry("trianglemesh")
    {
    }

    std::string TriangleMesh::toString() const
    {
      return "ospray::sg::TriangleMesh";
    }

    box3f TriangleMesh::bounds() const
    {
      box3f bounds = empty;
      if (hasChild("vertex")) {
        auto v = child("vertex").nodeAs<DataBuffer>();
        for (uint32_t i = 0; i < v->size(); i++)
          bounds.extend(v->get<vec3f>(i));
      }
      return bounds;
    }

    void TriangleMesh::preCommit(RenderContext &ctx)
    {
      // NOTE(jda) - how many buffers to we minimally _have_ to have?
      if (!hasChild("vertex") || !hasChild("index"))
        throw std::runtime_error("#osp.sg - error, invalid TriangleMesh!");

      Geometry::preCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(TriangleMesh);

  } // ::ospray::sg
} // ::ospray


