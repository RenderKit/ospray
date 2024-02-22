// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedLoadBalancer.h"
#include <rkcommon/tasking/AsyncTask.h>
#include <algorithm>
#include <limits>
#include <map>
#include "ObjectHandle.h"
#include "WriteMultipleTileOperation.h"
#include "camera/Camera.h"
#include "common/DistributedWorld.h"
#include "common/DynamicLoadBalancer.h"
#include "common/MPICommon.h"
#include "distributed/DistributedRenderer.h"
#include "fb/DistributedFrameBuffer.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/tracing/Tracing.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/getEnvVar.h"

namespace ospray {
namespace mpi {
using namespace mpicommon;
using namespace rkcommon;

DistributedLoadBalancer::DistributedLoadBalancer(ObjectHandle handle)
    : handle(handle)
{}

DistributedLoadBalancer::~DistributedLoadBalancer()
{
  handle.free();
}

std::pair<AsyncEvent, AsyncEvent> DistributedLoadBalancer::renderFrame(
    FrameBuffer *_fb,
    Renderer *_renderer,
    Camera *camera,
    World *_world,
    bool /*wait*/)
{
  auto *dfb = dynamic_cast<DistributedFrameBuffer *>(_fb);
  if (!dfb) {
    throw std::runtime_error(
        "Distributed Load Balancer only supports DistributedFrameBuffer!");
  }

  auto *world = dynamic_cast<DistributedWorld *>(_world);
  if (!world) {
    throw std::runtime_error(
        "Distributed Load Balancer only supports DistributedWorld!");
  }

  auto *renderer = dynamic_cast<DistributedRenderer *>(_renderer);
  if (!renderer) {
    if (world->allRegions.size() == 1) {
      renderFrameReplicated(dfb, _renderer, camera, world);
      return std::make_pair(AsyncEvent(), AsyncEvent());
    } else {
      throw std::runtime_error(
          "Distributed rendering requires a distributed renderer!");
    }
  }
  if (dfb->getLastRenderer() != renderer) {
    dfb->setTileOperation(renderer->tileOperation(), renderer);
  }

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("renderFrameDistributed", "DistributedLB"));

  dfb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(dfb, world);
  const size_t numRegions = world->allRegions.size();

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("determineTilesForFrame", "DistributedLB"));

  std::vector<std::vector<uint32_t>> regionTileIds(world->myRegionIds.size());
  const vec2i numTiles = dfb->getGlobalNumTiles();
  const vec2i fbSize = dfb->getNumPixels();
  for (size_t i = 0; i < world->myRegionIds.size(); ++i) {
    const size_t id = world->myRegionIds[i];
    const auto &r = world->allRegions[id];
    box3f proj = camera->projectBox(r);
    box2f screenRegion(vec2f(proj.lower) * fbSize, vec2f(proj.upper) * fbSize);

    // Pad the region a bit
    screenRegion.lower = max(screenRegion.lower - TILE_SIZE, vec2f(0.f));
    screenRegion.upper = min(screenRegion.upper + TILE_SIZE, vec2f(fbSize));

    // Skip regions that are completely behind the camera
    if (proj.upper.z < 0.f) {
      continue;
    }

    box2i tileRegion;
    tileRegion.lower = screenRegion.lower / TILE_SIZE;
    tileRegion.upper = vec2i(std::ceil(screenRegion.upper.x / TILE_SIZE),
        std::ceil(screenRegion.upper.y / TILE_SIZE));
    tileRegion.upper = min(tileRegion.upper, numTiles);
    for (int y = tileRegion.lower.y; y < tileRegion.upper.y; ++y) {
      for (int x = tileRegion.lower.x; x < tileRegion.upper.x; ++x) {
        const uint32_t tileIndex = x + y * numTiles.x;
        const auto &owners = world->regionOwners[id];
        const size_t numRegionOwners = owners.size();
        const size_t ownerRank =
            std::distance(owners.begin(), owners.find(workerRank()));

        // TODO: Can we do a better than round-robin over all tiles
        // assignment here? It could be that we end up not evenly dividing
        // the workload.
        const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;

        // If we're the owner and this tile isn't completed by adaptive
        // refinement, we need to render it
        if (regionTileOwner) {
          regionTileIds[i].push_back(tileIndex);
        }
      }
    }
  }

  // Set up the DFB sparse layers for each region we're rendering locally
  // We need one SparseFB layer in the DFB per region we're rendering locally
  dfb->setSparseFBLayerCount(world->myRegionIds.size());
  for (size_t i = 0; i < world->myRegionIds.size(); ++i) {
    // Layer 0 is the base "background" tiles, regions are rendered to
    // layers 1..# regions + 1
    auto *layer = dfb->getSparseFBLayer(i + 1);
    // Check if this layer already stores the tiles we want, and if so don't
    // reset it
    const utility::ArrayView<uint32_t> layerTiles = layer->getTileIDs();
    // TODO: it'd be nice if we had some other change tracking that would let us
    // know when we have different tiles rather than having to check the lists
    // of IDs against each other.
    // If this is a new FB, or the camera/scene has changed, we should set the
    // new tiles because they've probably changed. If nothing in the scene is
    // different from the previous render, then we don't need to set new tiles
    // because we're just accumulating.
    if (layerTiles.size() != regionTileIds[i].size()
        || !std::equal(
            layerTiles.begin(), layerTiles.end(), regionTileIds[i].begin())) {
      layer->setTiles(regionTileIds[i]);
    }
  }

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("renderLocalData", "DistributedLB");
  });

#ifdef OSPRAY_TARGET_SYCL
  /* For GPU: Need further refactoring to run rendering async for multiple
   * region per rank configs. Right now this is serial to avoid a race condition
   * on the SYCL queue.
   */
  tasking::serial_for(dfb->getSparseLayerCount(), [&](size_t layer)
#else
  tasking::parallel_for(dfb->getSparseLayerCount(), [&](size_t layer)
#endif
  {
    // Just render the background color and compute visibility information for
    // the tiles we own for layer "0"
    SparseFrameBuffer *sparseFb = dfb->getSparseFBLayer(layer);
    if (sparseFb->getNumTiles() == 0) {
      return;
    }

    RKCOMMON_IF_TRACING_ENABLED(
        rkcommon::tracing::beginEvent("setupLayerRendering", "DistributedLB"));

    // Explicitly avoiding std::vector<bool> because we need to match ISPC's
    // memory layout
    const utility::ArrayView<uint32_t> tileIDs = sparseFb->getTileIDs();

    // We use uint8 instead of bool to avoid hitting UB with differing "true"
    // values used by ISPC and C++
    BufferShared<uint8_t> regionVisible(
        sparseFb->getISPCDevice().getIspcrtContext(),
        numRegions * sparseFb->getNumTiles());
    std::memset(regionVisible.sharedPtr(), 0, regionVisible.size());

    // Compute visibility for the tasks we're rendering
    RKCOMMON_IF_TRACING_ENABLED({
      rkcommon::tracing::endEvent();
      rkcommon::tracing::beginEvent("computeRegionVisibility", "DistributedLB");
    });

    renderer->computeRegionVisibility(sparseFb,
        camera,
        world,
        regionVisible.sharedPtr(),
        perFrameData,
        sparseFb->getRenderTaskIDs());

    RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());

    // If we're rendering the background tiles send them over now
    if (layer == 0) {
      RKCOMMON_IF_TRACING_ENABLED(
          rkcommon::tracing::beginEvent("sendLayer0Tiles", "DistributedLB"));
      tasking::parallel_for(sparseFb->getNumTiles(), [&](size_t i) {
        // Don't send anything if this tile was finished due to adaptive
        // refinement
        if (dfb->tileError(tileIDs[i]) <= renderer->errorThreshold) {
          return;
        }

        // Just generate the bgtile properties the same way as the SparseFB
        // does to avoid having to read them back to the host
        Tile bgtile;
        bgtile.fbSize = sparseFb->getNumPixels();
        bgtile.rcp_fbSize = rcp(vec2f(sparseFb->getNumPixels()));
        bgtile.region = sparseFb->getTileRegion(tileIDs[i]);
        bgtile.accumID = 0;

        bgtile.sortOrder = std::numeric_limits<int32_t>::max();
        bgtile.generation = 0;
        bgtile.children = 0;
        bgtile.accumID = dfb->getFrameID();
        for (size_t r = 0; r < numRegions; ++r) {
          if (regionVisible[numRegions * i + r]) {
            ++bgtile.children;
          }
        }
        // Fill the tile with the background color
        // TODO: Should replace this with a smaller metadata message so we don't
        // need to send a full tile
        std::fill(
            bgtile.r, bgtile.r + TILE_SIZE * TILE_SIZE, renderer->bgColor.x);
        std::fill(
            bgtile.g, bgtile.g + TILE_SIZE * TILE_SIZE, renderer->bgColor.y);
        std::fill(
            bgtile.b, bgtile.b + TILE_SIZE * TILE_SIZE, renderer->bgColor.z);
        std::fill(
            bgtile.a, bgtile.a + TILE_SIZE * TILE_SIZE, renderer->bgColor.w);
        std::fill(bgtile.z,
            bgtile.z + TILE_SIZE * TILE_SIZE,
            std::numeric_limits<float>::infinity());
        dfb->setTile(bgtile);
      });
      RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
    } else {
      // If we're rendering a region, render it and send over the tile data
      RKCOMMON_IF_TRACING_ENABLED({
        rkcommon::tracing::beginEvent("renderRegionData", "DistributedLB");
        rkcommon::tracing::beginEvent("initRegionTaskIDs", "DistributedLB");
        rkcommon::tracing::setCounter(
            "regionTileCount", sparseFb->getNumTiles());
      });

      sparseFb->beginFrame();
      const size_t rid = world->myRegionIds[layer - 1];

      RKCOMMON_IF_TRACING_ENABLED({
        rkcommon::tracing::endEvent();
        rkcommon::tracing::beginEvent("renderRegionTasks", "DistributedLB");
      });
      renderer->renderRegionTasks(sparseFb,
          camera,
          world,
          world->allRegions[rid],
          perFrameData,
          sparseFb->getRenderTaskIDs());

      RKCOMMON_IF_TRACING_ENABLED({
        rkcommon::tracing::endEvent();
        rkcommon::tracing::beginEvent("readbackRegionTiles", "DistributedLB");
      });

      const auto tiles = sparseFb->getTiles();

      RKCOMMON_IF_TRACING_ENABLED({
        rkcommon::tracing::endEvent();
        rkcommon::tracing::beginEvent("sendRegionTiles", "DistributedLB");
      });
      tasking::parallel_for(tiles.size(), [&](size_t i) {
        if (!regionVisible[numRegions * i + rid]
            || dfb->tileError(tileIDs[i]) <= renderer->errorThreshold) {
          return;
        }
        Tile &regionTile = tiles[i];
        regionTile.generation = 1;
        regionTile.children = 0;
        regionTile.accumID = dfb->getFrameID();
        dfb->setTile(regionTile);
      });

      RKCOMMON_IF_TRACING_ENABLED({
        rkcommon::tracing::endEvent();
        rkcommon::tracing::endEvent();
      });
    }
  });
  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("waitUntilFinished", "DistributedLB");
  });

  dfb->waitUntilFinished();

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("postProcess", "DistributedLB");
  });

  // TODO: We can start to pipeline the post processing here,
  // but needs support from the rest of the async tasking from
  // MPI tasks. Right now we just run on a separate thread.
  dfb->postProcess(true);
  dfb->setCompletedEvent(OSP_FRAME_FINISHED);

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::endEvent();
  });

  return std::make_pair(AsyncEvent(), AsyncEvent());
}

void DistributedLoadBalancer::renderFrameReplicated(DistributedFrameBuffer *dfb,
    Renderer *renderer,
    Camera *camera,
    DistributedWorld *world)
{
  int enableStaticBalancer = 0;
  auto OSPRAY_STATIC_BALANCER =
      utility::getEnvVar<int>("OSPRAY_STATIC_BALANCER");

  enableStaticBalancer = OSPRAY_STATIC_BALANCER.value_or(0);

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("renderFrameReplicated", "DistributedLB"));

  std::shared_ptr<TileOperation> tileOperation = nullptr;
  if (dfb->getLastRenderer() != renderer) {
    tileOperation = std::make_shared<WriteMultipleTileOperation>();
    dfb->setTileOperation(tileOperation, renderer);
  } else {
    tileOperation = dfb->getTileOperation();
  }

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("startNewFrame", "DistributedLB"));

  dfb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(dfb, world);

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("renderLocalTiles", "DistributedLB");
  });

  if (enableStaticBalancer) {
    renderFrameReplicatedStaticLB(dfb, renderer, camera, world, perFrameData);
  } else {
    renderFrameReplicatedDynamicLB(dfb, renderer, camera, world, perFrameData);
  }
  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("waitUntilFinished", "DistributedLB");
  });

  dfb->waitUntilFinished();

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("postProcess", "DistributedLB");
  });

  // TODO: We can start to pipeline the post processing here,
  // but needs support from the rest of the async tasking from
  // MPI tasks. Right now we just run on a separate thread.
  dfb->postProcess(true);
  dfb->setCompletedEvent(OSP_FRAME_FINISHED);

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::endEvent();
  });
}

std::string DistributedLoadBalancer::toString() const
{
  return "ospray::mpi::Distributed";
}

void DistributedLoadBalancer::renderFrameReplicatedDynamicLB(
    DistributedFrameBuffer *dfb,
    Renderer *renderer,
    Camera *camera,
    DistributedWorld *world,
    void *perFrameData)
{
  bool askedForWork = false;

  const int ALLTILES = dfb->getGlobalTotalTiles();
  int NTILES = ALLTILES / workerSize();
  // NOTE(jda) - If all tiles do not divide evenly among all worker ranks
  //             (a.k.a. ALLTILES / worker.size has a remainder), then
  //             some ranks will have one extra tile to do. Thus NTILES
  //             is incremented if we are one of those ranks.
  if ((ALLTILES % workerSize()) > workerRank())
    NTILES++;

  // Do not pass all tiles at once, this way if other ranks want to steal
  // work, they can
  // We target 1/3rd of tiles per-round by default to balance between executing
  // work locally and allowing work to be stolen for load balancing.
  // This can be overridden by the environment variable
  // OSPRAY_MPI_LB_TILES_PER_ROUND
  const float percentTilesPerRound =
      utility::getEnvVar<float>("OSPRAY_MPI_LB_TILES_PER_ROUND")
          .value_or(0.33f);
  const int maxTilesPerRound = std::ceil(NTILES * percentTilesPerRound);
  const float minActiveTilesPercent =
      utility::getEnvVar<float>("OSPRAY_MPI_LB_MIN_ACTIVE_TILES")
          .value_or(0.25f);
  const int minActiveTiles = (ALLTILES / workerSize()) * minActiveTilesPercent;

  // Avoid division by 0 for the case that this rank doesn't have any tiles
  int numRounds = 0;
  int tilesPerRound = 0;
  int remainTiles = 0;
  if (NTILES > 0) {
    numRounds = std::max(NTILES / maxTilesPerRound, 1);
    tilesPerRound = NTILES / numRounds;
    remainTiles = NTILES % numRounds;
  }
  int terminatedTiles = 0;

  auto dynamicLB = make_unique<DynamicLoadBalancer>(handle, ALLTILES);

  mpicommon::barrier(dfb->getMPIGroup().comm).wait();
  int totalActiveTiles = ALLTILES;
  // push current work to workQueue
  for (int i = 0; i < numRounds; i++) {
    Work myWork;
    myWork.ntasks = tilesPerRound;
    myWork.offset = i * tilesPerRound;
    myWork.ownerRank = workerRank();
    dynamicLB->addWork(myWork);
  }

  // Extra round for any remainder tiles
  if (remainTiles > 0) {
    Work myWork;
    myWork.ntasks = remainTiles;
    myWork.offset = numRounds * tilesPerRound;
    myWork.ownerRank = workerRank();
    dynamicLB->addWork(myWork);
  }

  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::beginEvent(
      "localRenderLoop", "DistributedLB,Dynamic"));

  const int sparseFbChannelFlags =
      dfb->getChannelFlags() & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE);

  auto sparseFb = rkcommon::make_unique<SparseFrameBuffer>(dfb->getISPCDevice(),
      dfb->getNumPixels(),
      dfb->getColorBufferFormat(),
      sparseFbChannelFlags);

  while (0 < totalActiveTiles) {
    Work currentWorkItem;
    if (0 < dynamicLB->getWorkSize()) {
      currentWorkItem = dynamicLB->getWorkItemFront();
      askedForWork = false;
    }

    if (0 < currentWorkItem.ntasks) {
      RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::beginEvent(
          "initTaskTileIDs", "DistributedLB,Dynamic"));
      // Allocate a sparse framebuffer to hold the render output data
      std::vector<uint32_t> taskTileIDs;
      taskTileIDs.reserve(currentWorkItem.ntasks);
      for (int i = 0; i < currentWorkItem.ntasks; ++i) {
        const int tileID = (i + currentWorkItem.offset) * workerSize()
            + currentWorkItem.ownerRank;
        // Only render tiles if they haven't reached the error threshold
        if (dfb->tileError(tileID) > renderer->errorThreshold) {
          taskTileIDs.push_back(tileID);
        }
      }
      RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
      // If all the tiles we were assigned are already finished due to adaptive
      // termination we have nothing to render locally so just mark the tasks
      // complete
      if (!taskTileIDs.empty()) {
        RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::beginEvent(
            "setSparseFBTiles", "DistributedLB,Dynamic"));
        // Set the tiles and fill the right accumID for them
        sparseFb->setTiles(taskTileIDs);
        sparseFb->beginFrame();
        sparseFb->setFrameID(dfb->getFrameID());

        RKCOMMON_IF_TRACING_ENABLED({
          rkcommon::tracing::endEvent();
          rkcommon::tracing::beginEvent("renderTasks", "DistributedLB,Dynamic");
        });

        renderer->renderTasks(sparseFb.get(),
            camera,
            world,
            perFrameData,
            sparseFb->getRenderTaskIDs(
                renderer->errorThreshold, renderer->spp));

        RKCOMMON_IF_TRACING_ENABLED({
          rkcommon::tracing::endEvent();
          rkcommon::tracing::beginEvent(
              "readbackTiles", "DistributedLB,Dynamic");
        });

        // TODO: Now the tile setting happens as a bulk-sync operation after
        // rendering, because we still need to send them through the compositing
        // pipeline. The ISPC-side rendering code doesn't know about this and in
        // the future wouldn't be able to do it
        // One option with the Dynamic LB would be to at least ping-poing
        // sparseFb's, one is being rendered into while tiles from the previous
        // task set are sent out
        const utility::ArrayView<Tile> tiles = sparseFb->getTiles();

        RKCOMMON_IF_TRACING_ENABLED({
          rkcommon::tracing::endEvent();
          rkcommon::tracing::beginEvent("sendTiles", "DistributedLB,Dynamic");
        });

        tasking::parallel_for(tiles.size(), [&](size_t i) {
          Tile &tile = tiles[i];
          tile.accumID = dfb->getFrameID();
          dfb->setTile(tile);
        });
      }
      RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());

      // Mark the set of tasks as complete
      dynamicLB->sendTerm(currentWorkItem.ntasks);
      terminatedTiles = terminatedTiles + currentWorkItem.ntasks;
    }

    if (0 < terminatedTiles) {
      dynamicLB->updateActiveTasks(terminatedTiles);
    }

    totalActiveTiles = dynamicLB->getActiveTasks();
    terminatedTiles = 0;

    // if the total active tiles over all workers is more than some min,
    // request work
    if (currentWorkItem.ntasks <= 0 && minActiveTiles < totalActiveTiles
        && !askedForWork) {
      dynamicLB->requestWork();
      askedForWork = true;
    }
  }

  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());

  // We need to wait for the other ranks to finish here to keep our local DLB
  // alive to respond to any work requests that come in while other ranks
  // finish their final local set of tasks
  mpicommon::barrier(dfb->getMPIGroup().comm).wait();
}

void DistributedLoadBalancer::renderFrameReplicatedStaticLB(
    DistributedFrameBuffer *dfb,
    Renderer *renderer,
    Camera *camera,
    DistributedWorld *world,
    void *perFrameData)
{
  SparseFrameBuffer *ownedTilesFb = dfb->getSparseFBLayer(0);

  // Note: these views are already in USM
  const utility::ArrayView<uint32_t> tileIDs = ownedTilesFb->getTileIDs();

  RKCOMMON_IF_TRACING_ENABLED(
      rkcommon::tracing::beginEvent("localRenderLoop", "DistributedLB,Static"));
  renderer->renderTasks(ownedTilesFb,
      camera,
      world,
      perFrameData,
      ownedTilesFb->getRenderTaskIDs(renderer->errorThreshold, renderer->spp));

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("readbackTiles", "DistributedLB,Static");
  });

  const utility::ArrayView<Tile> tiles = ownedTilesFb->getTiles();

  RKCOMMON_IF_TRACING_ENABLED({
    rkcommon::tracing::endEvent();
    rkcommon::tracing::beginEvent("sendTiles", "DistributedLB,Static");
  });

  // TODO: Now the tile setting happens as a bulk-sync operation after
  // rendering, because we still need to send them through the compositing
  // pipeline. The ISPC-side rendering code doesn't know about this and in the
  // future wouldn't be able to do it
  tasking::parallel_for(tiles.size(), [&](size_t i) {
    // Don't send anything if this tile was finished due to adaptive
    // refinement
    if (dfb->tileError(tileIDs[i]) <= renderer->errorThreshold) {
      return;
    }
    dfb->setTile(tiles[i]);
  });
  RKCOMMON_IF_TRACING_ENABLED(rkcommon::tracing::endEvent());
}

} // namespace mpi
} // namespace ospray
