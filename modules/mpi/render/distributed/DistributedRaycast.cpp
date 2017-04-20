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

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// ospray
#include "DistributedRaycast.h"
#include "../../fb/DistributedFrameBuffer.h"
// ispc exports
#include "DistributedRaycast_ispc.h"

namespace ospray {
  namespace mpi {

    // DistributedRaycastRenderer definitions /////////////////////////////////

    DistributedRaycastRenderer::DistributedRaycastRenderer()
    {
      ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);
    }

    void DistributedRaycastRenderer::commit()
    {
      Renderer::commit();
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 fbChannelFlags)
    {
      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);

      dfb->setFrameMode(DistributedFrameBuffer::Z_COMPOSITE);

      dfb->beginFrame();
      dfb->startNewFrame(errorThreshold);

      auto *perFrameData = Renderer::beginFrame(dfb);

      //TODO: move to a LoadBalancer instead?

      parallel_for(dfb->getTotalTiles(), [&](int taskIndex) {
        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = taskIndex / numTiles_x;
        const size_t tile_x = taskIndex - tile_y*numTiles_x;
        const vec2i tileID(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileID);

        if (dfb->tileError(tileID) <= errorThreshold)
          return;

        Tile __aligned(64) tile(tileID, dfb->size, accumID);

        const int NUM_JOBS = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
        parallel_for(NUM_JOBS, [&](int tIdx) {
          renderTile(perFrameData, tile, tIdx);
        });

        fb->setTile(tile);
      });

      dfb->waitUntilFinished();
      Renderer::endFrame(nullptr, fbChannelFlags);

      return dfb->endFrame(0.f);//TODO: can report error threshold here?
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
    }

  } // ::ospray::mpi
} // ::ospray

