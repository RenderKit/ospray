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

#include <memory>
#include <vector>
#include "../../fb/TileOperation.h"

namespace ospray {
  struct AlphaCompositeTileOperation : public TileOperation
  {
    std::shared_ptr<LiveTileOperation>
    makeTile(DistributedFrameBuffer *dfb,
             const vec2i &tileBegin,
             size_t tileID,
             size_t ownerID) override;

    std::string toString() const;
  };
}


