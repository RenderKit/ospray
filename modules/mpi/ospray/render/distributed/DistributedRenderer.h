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

#include "../../common/DistributedWorld.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "../../fb/TileOperation.h"
#include "camera/Camera.h"
#include "render/Renderer.h"

namespace ospray {
namespace mpi {

struct RegionInfo
{
  int numRegions = 0;
  Region *regions = nullptr;
  bool *regionVisible = nullptr;
};

struct DistributedRenderer : public Renderer
{
  void computeRegionVisibility(DistributedFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      bool *regionVisible,
      void *perFrameData,
      Tile &tile,
      size_t jobID) const;

  void renderRegionToTile(DistributedFrameBuffer *fb,
      Camera *camera,
      DistributedWorld *world,
      const Region &region,
      void *perFrameData,
      Tile &tile,
      size_t jobID) const;

  virtual std::shared_ptr<TileOperation> tileOperation() = 0;
};
} // namespace mpi
} // namespace ospray
