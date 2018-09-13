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

#include "StreamLines.h"
#include "../common/Data.h"

namespace ospray {
  namespace sg {

    StreamLines::StreamLines() : Geometry("streamlines")
    {
      createChild("radius", "float", 0.01f,
                  NodeFlags::valid_min_max).setMinMax(1e-20f, 1e20f);
      createChild("smooth", "bool", false);
    }

    std::string StreamLines::toString() const
    {
      return "ospray::sg::StreamLines";
    }

    box3f StreamLines::computeBounds() const
    {
      box3f bbox = bounds();

      if (bbox != box3f(empty))
        return bbox;

      auto vtx = child("vertex").nodeAs<DataBuffer>()->baseAs<vec3fa>();
      auto idx = child("index").nodeAs<DataBuffer>();
      // TODO: radius varies potentially per vertex
      auto radius = child("radius").valueAs<float>();
      for (uint32_t e = 0; e < idx->size(); e++) {
        int i = idx->get<int>(e);
        bbox.extend(vtx[i] - radius);
        bbox.extend(vtx[i] + radius);
        bbox.extend(vtx[i+1] - radius);
        bbox.extend(vtx[i+1] + radius);
      }

      child("bounds") = bbox;

      return bbox;
    }

    void StreamLines::preCommit(RenderContext &ctx)
    {
      if (!hasChild("vertex") || !hasChild("index"))
        throw std::runtime_error("#osp.sg - error, invalid StreamLines!");

      Geometry::preCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(StreamLines);

  } // ::ospray::sg
} // ::ospray
