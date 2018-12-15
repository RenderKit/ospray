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

#include "QuadMesh.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    QuadMesh::QuadMesh() : Geometry("quads")
    {
    }

    std::string QuadMesh::toString() const
    {
      return "ospray::sg::QuadMesh";
    }

    box3f QuadMesh::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      if (hasChild("vertex")) {
        auto v = child("vertex").nodeAs<DataBuffer>();
        for (uint32_t i = 0; i < v->size(); i++)
          bbox.extend(v->get<vec3f>(i));
      }
      child("bounds") = bbox;
      return bbox;
    }

    void QuadMesh::preCommit(RenderContext &ctx)
    {
      // NOTE(jda) - how many buffers to we minimally _have_ to have?
      if (!hasChild("vertex"))
          throw std::runtime_error("#osp.sg - error, QuadMesh has no 'vertex' field!");
      if (!hasChild("index"))
          throw std::runtime_error("#osp.sg - error, QuadMesh has no 'index' field!");

      Geometry::preCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(QuadMesh);

  } // ::ospray::sg
} // ::ospray


