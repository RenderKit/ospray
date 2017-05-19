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
#include "common/Data.h"
// ospray
#include "DistributedRaycast.h"
#include "../../common/DistributedModel.h"
#include "../MPILoadBalancer.h"
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
      DistributedModel *distribModel = dynamic_cast<DistributedModel*>(model);
      if (distribModel) {
        ispc::DistributedRaycastRenderer_setClipBoxes(ispcEquivalent,
            (ispc::box3f*)distribModel->myClipBoxes.data(),
            (ispc::box3f*)distribModel->othersClipBoxes.data());
      } else {
        // TODO: Should this be a hard error and we just throw?
        static WarnOnce warning("Using DistributedRaycastRender but not a distributed model!?");
      }
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 channelFlags)
    {
      using namespace mpicommon;

      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
      dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);
      dfb->startNewFrame(errorThreshold);
      dfb->beginFrame();

      auto *perFrameData = beginFrame(dfb);

      tasking::parallel_for(dfb->getTotalTiles(), [&](int taskIndex) {
        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = taskIndex / numTiles_x;
        const size_t tile_x = taskIndex - tile_y*numTiles_x;
        const vec2i tileID(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileID);
        const bool tileOwner = (taskIndex % numGlobalRanks()) == globalRank();

        if (dfb->tileError(tileID) <= errorThreshold)
          return;

        // TODO: Call render for each box we own, and pass through 'perFrameData'
        // some per tile data so we can get out information about which other
        // boxes of our own and others projected to this tile. This could be passed
        // just on the first box we render, then we pass null for the remaining to skip
        // doing the tests again.
        Tile __aligned(64) tile(tileID, dfb->size, accumID);

        const int NUM_JOBS = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;
        tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
          renderTile(perFrameData, tile, tIdx);
        });

        if (tileOwner) {
          tile.generation = 0;
          tile.children = numGlobalRanks() - 1;
          // TODO: If we're the owner, fill a tile with the renderer's background color
          // and send that as well.
        } else {
          tile.generation = 1;
          tile.children = 0;
        }

        fb->setTile(tile);
      });

      dfb->waitUntilFinished();
      endFrame(nullptr, channelFlags);

      return dfb->endFrame(errorThreshold);
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
    }

    OSP_REGISTER_RENDERER(DistributedRaycastRenderer, mpi_raycast);

  } // ::ospray::mpi
} // ::ospray

