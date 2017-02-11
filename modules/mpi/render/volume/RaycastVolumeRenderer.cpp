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

// ospray
#include "lights/Light.h"
#include "common/Data.h"
#include "RaycastVolumeRenderer.h"
#include "RaycastVolumeMaterial.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"

// ours
// ispc exports
#include "RaycastVolumeRenderer_ispc.h"

#include "mpiCommon/MPICommon.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "../../volume/DataDistributedBlockedVolume.h"
#include "render/LoadBalancer.h"

#define TILE_CACHE_SAFE_MUTEX 0

namespace ospray {

  Material *RaycastVolumeRenderer::createMaterial(const char *type)
  {
    UNUSED(type);
    return new RaycastVolumeMaterial;
  }

  struct CacheForBlockTiles
  {
    CacheForBlockTiles(size_t numBlocks)
      : numBlocks(numBlocks), blockTile(new Tile *[numBlocks])
    {
      for (size_t i = 0; i < numBlocks; i++) blockTile[i] = nullptr;
    }

    ~CacheForBlockTiles()
    {
      for (size_t i = 0; i < numBlocks; i++)
        if (blockTile[i]) delete blockTile[i];

      delete[] blockTile;
    }

    Tile *allocTile()
    {
      float infinity = std::numeric_limits<float>::infinity();
      Tile *tile = new Tile;
      for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) tile->r[i] = 0.f;
      for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) tile->g[i] = 0.f;
      for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) tile->b[i] = 0.f;
      for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) tile->a[i] = 0.f;
      for (int i = 0; i < TILE_SIZE*TILE_SIZE; i++) tile->z[i] = infinity;
      return tile;
    }

    Tile *getTileForBlock(size_t blockID)
    {
#if TILE_CACHE_SAFE_MUTEX
      mutex.lock();
      Tile *tile = blockTile[blockID];
      if (tile == nullptr) {
        blockTile[blockID] = tile = allocTile();
      }
      mutex.unlock();
      return tile;
#else
      Tile *tile = blockTile[blockID];
      if (tile != nullptr) return tile;
      mutex.lock();
      tile = blockTile[blockID];
      if (tile == nullptr) {
        blockTile[blockID] = tile = allocTile();
      }
      mutex.unlock();
      return tile;
#endif
    }

    Mutex mutex;
    size_t numBlocks;
    Tile *volatile *blockTile;
  };

  /*! extern exported function so even ISPC code can access this cache */
  extern "C" Tile *
  CacheForBlockTiles_getTileForBlock(CacheForBlockTiles *cache,
                                     uint32 blockID)
  {
    return cache->getTileForBlock(blockID);
  }

  struct DPRenderTask
  {
    mutable Ref<Renderer>     renderer;
    mutable Ref<FrameBuffer>  fb;
    size_t                    numTiles_x;
    size_t                    numTiles_y;
    uint32                    channelFlags;
    const DataDistributedBlockedVolume *dpv;

    void operator()(int taskIndex) const;
  };

  void DPRenderTask::operator()(int taskIndex) const
  {
    const size_t tileID = taskIndex;
    const size_t tile_y = taskIndex / numTiles_x;
    const size_t tile_x = taskIndex - tile_y*numTiles_x;
    const vec2i tileId(tile_x, tile_y);
    const int32 accumID = fb->accumID(tileId);
    Tile bgTile(tileId, fb->size, accumID);
    Tile fgTile(bgTile);

    size_t numBlocks = dpv->numDDBlocks;
    CacheForBlockTiles blockTileCache(numBlocks);
    bool *blockWasVisible = STACK_BUFFER(bool, numBlocks);

    for (size_t i = 0; i < numBlocks; i++)
      blockWasVisible[i] = false;

    bool renderForeAndBackground =
        (taskIndex % mpi::getWorkerCount()) == mpi::getWorkerRank();

    const int numJobs = (TILE_SIZE*TILE_SIZE)/RENDERTILE_PIXELS_PER_JOB;

    parallel_for(numJobs, [&](int tid){
      ispc::DDDVRRenderer_renderTile(renderer->getIE(),
                                     (ispc::Tile&)fgTile,
                                     (ispc::Tile&)bgTile,
                                     &blockTileCache,
                                     numBlocks,
                                     dpv->ddBlock,
                                     blockWasVisible,
                                     tileID,
                                     mpi::getWorkerRank(),
                                     renderForeAndBackground,
                                     tid);
    });


    if (renderForeAndBackground) {
      // this is a tile owned by me - i'm responsible for writing
      // generaition #0, and telling the fb how many more tiles will
      // be coming in generation #1

      size_t totalBlocksInTile = 0;
      for (size_t blockID = 0; blockID < numBlocks; blockID++) {
        if (blockWasVisible[blockID])
          totalBlocksInTile++;
      }

      size_t nextGenTiles
          = 1 /* expect one additional tile for background tile. */
          + totalBlocksInTile /* plus how many blocks map to this
                                     tile, IN TOTAL (ie, INCLUDING blocks
                                     on other nodes)*/;
      // printf("rank %i total tiles in tile %i is %i\n",
      //        core::getWorkerRank(),taskIndex,nextGenTiles);

      // set background tile
      bgTile.generation = 0;
      bgTile.children   = nextGenTiles;
      fb->setTile(bgTile);

      // set foreground tile
      fgTile.generation = 1;
      fgTile.children   = 0; //nextGenTiles-1;
      fb->setTile(fgTile);
      // all other tiles for gen #1 will be set below, no matter whether
      // it's mine or not
    }

    // now, send all block cache tiles that were generated on this
    // node back to master, too. for this, it doesn't matter if this
    // is our tile or not; it's the job of the owner of this tile to
    // tell the DFB how many tiles will arrive for the final thile
    // _across_all_clients_, but we only have to send ours (assuming
    // that all clients together send exactly as many as the owner
    // told the DFB to expect)
    for (size_t blockID = 0; blockID < numBlocks; blockID++) {
      Tile *tile = blockTileCache.blockTile[blockID];
      if (tile == nullptr)
        continue;

      tile->region = bgTile.region;
      tile->fbSize = bgTile.fbSize;
      tile->rcp_fbSize = bgTile.rcp_fbSize;
      tile->accumID = accumID;
      tile->generation = 1;
      tile->children   = 0; //nextGenTile-1;

      fb->setTile(*tile);
    }
  }

  /*! try if we are running in data-parallel mode, and if
    data-parallel is even required. if not (eg, if there's no
    data-parallel volumes in the scene) return nullptr and render only
    in regular mode; otherwise, compute some precomputations and
    return pointer to that (which'll switch the main renderframe fct
    to render data parallel) */
  float RaycastVolumeRenderer::renderFrame(FrameBuffer *fb,
                                           const uint32 channelFlags)
  {
    using DDBV = DataDistributedBlockedVolume;
    std::vector<const DDBV*> ddVolumeVec;
    for (size_t volumeID = 0; volumeID < model->volume.size(); volumeID++) {
      const DDBV* ddv = dynamic_cast<const DDBV*>(model->volume[volumeID].ptr);
      if (!ddv) continue;
      ddVolumeVec.push_back(ddv);
    }

    if (ddVolumeVec.empty()) {
      static bool printed = false;
      if (!printed) {
        cout << "no data parallel volumes, rendering in traditional"
             << " raycast_volume_render mode" << endl;
        printed = true;
      }

      return Renderer::renderFrame(fb,channelFlags);
    }

    // =======================================================
    // OK, we _need_ data-parallel rendering ....
    static bool printed = false;
    if (!printed) {
      std::cout << "#dvr: at least one dp volume"
                   " -> needs data-parallel rendering ..." << std::endl;
      printed = true;
    }

    // check if we're even in mpi parallel mode (can't do
    // data-parallel otherwise)
    if (!ospray::mpi::isMpiParallel()) {
      throw std::runtime_error("#dvr: need data-parallel rendering, "
                               "but not running in mpi mode!?");
    }

    // switch (distributed) frame buffer into compositing mode
    DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
    if (!dfb) {
      throw std::runtime_error("OSPRay data parallel rendering error. "
                               "this is a data-parallel scene, but we're "
                               "not using the distributed frame buffer!?");
    }

    dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);

    Renderer::beginFrame(fb);

    dfb->startNewFrame(errorThreshold);

    if (ddVolumeVec.size() > 1) {
      /* note: our assumption below is that all blocks together are
         contiguous, and fill a convex region (ie, any point on a
         given ray is either entirely before any block, entirely
         behind any block, or inside one of the blocks) - if we have
         multiple data distributed volumes that is no longer the case,
         so we're not currently supporting this ... */
      throw std::runtime_error("currently supporting only ONE data parallel "
                               "volume in scene");
    }

    // create the render task
    DPRenderTask renderTask;
    renderTask.fb = fb;
    renderTask.renderer = this;
    renderTask.numTiles_x = divRoundUp(dfb->size.x, TILE_SIZE);
    renderTask.numTiles_y = divRoundUp(dfb->size.y, TILE_SIZE);
    renderTask.channelFlags = channelFlags;
    renderTask.dpv = ddVolumeVec[0];

    size_t NTASKS = renderTask.numTiles_x * renderTask.numTiles_y;
    parallel_for(NTASKS, std::move(renderTask));

    dfb->waitUntilFinished();
    Renderer::endFrame(nullptr, channelFlags);
    return fb->endFrame(0.f);
  }


  void RaycastVolumeRenderer::commit()
  {
    // Create the equivalent ISPC RaycastVolumeRenderer object.
    if (ispcEquivalent == nullptr) {
      ispcEquivalent = ispc::RaycastVolumeRenderer_createInstance();
    }

    // Set the lights if any.
    Data *lightsData = (Data *)getParamData("lights", nullptr);

    lights.clear();

    if (lightsData) {
      for (size_t i=0; i<lightsData->size(); i++)
        lights.push_back(((Light **)lightsData->data)[i]->getIE());
    }

    ispc::RaycastVolumeRenderer_setLights(ispcEquivalent,
                                          lights.empty() ? nullptr : &lights[0],
                                          lights.size());

    // Initialize state in the parent class, must be called after the ISPC
    // object is created.
    Renderer::commit();
  }

  // A renderer type for volumes with embedded surfaces.
  OSP_REGISTER_RENDERER(RaycastVolumeRenderer, raycast_volume_renderer);
  OSP_REGISTER_RENDERER(RaycastVolumeRenderer, dvr);

} // ::ospray

