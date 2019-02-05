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

#pragma once

#include "common/OSPCommon.h"
#include <vector>

namespace ospray {

  // manages error per tile and adaptive regions, for variance estimation
  class OSPRAY_SDK_INTERFACE TileError
  {
    public:
      TileError(const vec2i &numTiles);
      ~TileError();
      void clear();
      float operator[](const vec2i &tile) const;
      void update(const vec2i &tile, const float error);
      float refine(const float errorThreshold);

    protected:
      vec2i numTiles;
      int tiles;
      float *tileErrorBuffer; // holds error per tile
      // image regions (in #tiles) which do not yet estimate the error on
      // per-tile base
      std::vector<box2i> errorRegion;
  };
} // ::ospray
