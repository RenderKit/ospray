// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DistributedLoadBalancer.h"
#include <algorithm>
#include <limits>
#include <map>
#include "../common/DistributedWorld.h"
#include "../common/DynamicLoadBalancer.h"
#include "../fb/DistributedFrameBuffer.h"
#include "WriteMultipleTileOperation.h"
#include "camera/Camera.h"
#include "common/MPICommon.h"
#include "common/Profiling.h"
#include "distributed/DistributedRenderer.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/getEnvVar.h"

namespace ospray {
namespace mpi {
using namespace mpicommon;
using namespace rkcommon;

DistributedLoadBalancer::DistributedLoadBalancer() {}

DistributedLoadBalancer::~DistributedLoadBalancer()
{
  handle.free();
}
void DistributedLoadBalancer::renderFrame(
    FrameBuffer *_fb, Renderer *_renderer, Camera *camera, World *_world)
{
  auto *dfb = dynamic_cast<DistributedFrameBuffer *>(_fb);

  auto *world = dynamic_cast<DistributedWorld *>(_world);
  if (!world) {
    throw std::runtime_error(
        "Distributed Load Balancer only supports DistributedWorld!");
  }

  auto *renderer = dynamic_cast<DistributedRenderer *>(_renderer);
  if (!renderer) {
    if (world->allRegions.size() == 1) {
      renderFrameReplicated(dfb, _renderer, camera, world);
      return;
    } else {
      throw std::runtime_error(
          "Distributed rendering requires a "
          "distributed renderer!");
    }
  }
  if (dfb->getLastRenderer() != renderer) {
    dfb->setTileOperation(renderer->tileOperation(), renderer);
  }

  dfb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(dfb, world);
  const size_t numRegions = world->allRegions.size();

#ifdef ENABLE_PROFILING
  ProfilingPoint start = ProfilingPoint();
#endif

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
    const auto &layerTiles = layer->getTileIDs();
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

#ifdef ENABLE_PROFILING
  ProfilingPoint end = ProfilingPoint();
  std::cout << "Initial tile for frame determination "
            << elapsedTimeMs(start, end) << "ms\n";
#endif

  // Note: Later this will be a set of tasks we enqueue at the same time and
  // want to run in parallel to each other. For now this still runs as a
  // parallel for here so we don't end up having to do a more sync/blocking
  // style render
  tasking::parallel_for(dfb->getSparseLayerCount(), [&](size_t layer) {
    // Just render the background color and compute visibility information for
    // the tiles we own for layer "0"
    SparseFrameBuffer *sparseFb = dfb->getSparseFBLayer(layer);

    // Explicitly avoiding std::vector<bool> because we need to match ISPC's
    // memory layout
    const auto &tiles = sparseFb->getTiles();
    const auto &tileIDs = sparseFb->getTileIDs();

    // We use uint8 instead of bool to avoid hitting UB with differing "true"
    // values used by ISPC and C++
    std::vector<uint8_t> regionVisible(numRegions * tiles.size(), 0);

    // Compute visibility for the tasks we're rendering
    auto renderTaskIDs = sparseFb->getRenderTaskIDs();

    renderer->computeRegionVisibility(sparseFb,
        camera,
        world,
        regionVisible.data(),
        perFrameData,
        renderTaskIDs);

    // If we're rendering the background tiles send them over now
    if (layer == 0) {
      tasking::parallel_for(tiles.size(), [&](size_t i) {
        // Don't send anything if this tile was finished due to adaptive
        // refinement
        if (dfb->tileError(tileIDs[i]) <= renderer->errorThreshold) {
          return;
        }

        // TODO: Would be nice to not copy here, but not sure about makign the
        // tiles modifiable either in SparseFrameBuffer::getTile.
        Tile bgtile = tiles[i];

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
    } else {
      // If we're rendering a region, render it and send over the tile data

      // Remove any tasks for tiles that we found the region isn't actually
      // visible to. The region projection is conservative in its bounds, so
      // we can often remove a substantial number of tasks, resulting in better
      // utilization. This also removes tasks that have completed due to
      // adaptive refinement
      const size_t rid = world->myRegionIds[layer - 1];
      std::vector<uint32_t> activeTasks(
          renderTaskIDs.begin(), renderTaskIDs.end());
      auto removeTasks = std::partition(
          activeTasks.begin(), activeTasks.end(), [&](const uint32_t taskID) {
            const uint32_t tileIdx = sparseFb->getTileIndexForTask(taskID);
            return regionVisible[numRegions * tileIdx + rid]
                && dfb->tileError(tileIDs[tileIdx]) > renderer->errorThreshold;
          });
      activeTasks.erase(removeTasks, activeTasks.end());

      renderer->renderRegionTasks(sparseFb,
          camera,
          world,
          world->allRegions[rid],
          perFrameData,
          utility::ArrayView<uint32_t>(activeTasks));

      tasking::parallel_for(tiles.size(), [&](size_t i) {
        if (!regionVisible[numRegions * i + rid]
            || dfb->tileError(tileIDs[i]) <= renderer->errorThreshold) {
          return;
        }
        // TODO: Would be nice to not copy here, but not sure about making
        // the tiles modifiable either in SparseFrameBuffer::getTile.
        Tile regionTile = tiles[i];

        regionTile.generation = 1;
        regionTile.children = 0;
        regionTile.accumID = dfb->getFrameID();
        dfb->setTile(regionTile);
      });
    }
  });

  dfb->waitUntilFinished();
  renderer->endFrame(dfb, perFrameData);

  dfb->endFrame(renderer->errorThreshold, camera);
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

  std::shared_ptr<TileOperation> tileOperation = nullptr;
  if (dfb->getLastRenderer() != renderer) {
    tileOperation = std::make_shared<WriteMultipleTileOperation>();
    dfb->setTileOperation(tileOperation, renderer);
  } else {
    tileOperation = dfb->getTileOperation();
  }

#ifdef ENABLE_PROFILING
  ProfilingPoint start;
#endif
  dfb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(dfb, world);
#ifdef ENABLE_PROFILING
  ProfilingPoint end;
  std::cout << "Start new frame took: " << elapsedTimeMs(start, end) << "ms\n";
#endif

  if (enableStaticBalancer) {
    renderFrameReplicatedStaticLB(dfb, renderer, camera, world, perFrameData);
  } else {
    renderFrameReplicatedDynamicLB(dfb, renderer, camera, world, perFrameData);
  }

#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Render loop took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";

  start = ProfilingPoint();
#endif

  dfb->waitUntilFinished();

#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Wait finished took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";

  start = ProfilingPoint();
#endif

  renderer->endFrame(dfb, perFrameData);
  dfb->endFrame(renderer->errorThreshold, camera);

#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "End frame took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";
#endif
}

std::string DistributedLoadBalancer::toString() const
{
  return "ospray::mpi::Distributed";
}

void DistributedLoadBalancer::setObjectHandle(ObjectHandle &handle_)
{
  handle = handle_;
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

  // Do not pass all tiles at once, this way if other ranks want to steal work,
  // they can
  const int maxTilesPerRound = 20;
  const int numRounds = std::max(NTILES / maxTilesPerRound, 1);
  const int tilesPerRound = NTILES / numRounds;
  const int remainTiles = NTILES % numRounds;
  const int minActiveTiles = (ALLTILES / workerSize()) * 0.25;
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

#ifdef ENABLE_PROFILING
  start = ProfilingPoint();
#endif

  const int sparseFbChannelFlags =
      dfb->getChannelFlags() & ~(OSP_FB_ACCUM | OSP_FB_VARIANCE);
  const bool sparseFbTrackAccumIDs = dfb->getChannelFlags() & OSP_FB_ACCUM;

  auto sparseFb = rkcommon::make_unique<SparseFrameBuffer>(dfb->getNumPixels(),
      dfb->getColorBufferFormat(),
      sparseFbChannelFlags,
      sparseFbTrackAccumIDs);

  while (0 < totalActiveTiles) {
    Work currentWorkItem;
    if (0 < dynamicLB->getWorkSize()) {
      currentWorkItem = dynamicLB->getWorkItemFront();
      askedForWork = false;
    }

    if (0 < currentWorkItem.ntasks) {
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
      sparseFb->setTiles(taskTileIDs);

      // Set the right accumID for the tiles we're going to render
      // TODO: would be nice if there was a more efficient way to run this as
      // well.
      if (sparseFbTrackAccumIDs) {
        for (uint32_t i = 0; i < sparseFb->getTotalRenderTasks(); ++i) {
          sparseFb->setTaskAccumID(i, dfb->getFrameID());
        }
      }

      renderer->renderTasks(sparseFb.get(),
          camera,
          world,
          perFrameData,
          sparseFb->getRenderTaskIDs());

      // TODO: Now the tile setting happens as a bulk-sync operation after
      // rendering, because we still need to send them through the compositing
      // pipeline. The ISPC-side rendering code doesn't know about this and in
      // the future wouldn't be able to do it
      // One option with the Dynamic LB would be to at least ping-poing
      // sparseFb's, one is being rendered into while tiles from the previous
      // task set are sent out
      const auto &tiles = sparseFb->getTiles();
      tasking::serial_for(tiles.size(), [&](size_t i) {
        // TODO: Same note as distributed case, would be nice here to not have
        // to copy the tile to change the accum ID.
        Tile tile = tiles[i];
        tile.accumID = dfb->getFrameID();
        dfb->setTile(tile);
      });
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
  // We need to wait for the other ranks to finish here to keep our local DLB
  // alive to respond to any work requests that come in while other ranks finish
  // their final local set of tasks
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

  const auto &tiles = ownedTilesFb->getTiles();
  const auto &tileIDs = ownedTilesFb->getTileIDs();
  auto renderTaskIDs = ownedTilesFb->getRenderTaskIDs();

  if (renderer->errorThreshold > 0.f) {
    std::vector<uint32_t> activeTasks;
    for (auto &i : renderTaskIDs) {
      const uint32_t tileID = tileIDs[ownedTilesFb->getTileIndexForTask(i)];
      const float error = dfb->tileError(tileID);
      if (error > renderer->errorThreshold) {
        activeTasks.push_back(i);
      }
    }

    renderer->renderTasks(ownedTilesFb,
        camera,
        world,
        perFrameData,
        utility::ArrayView<uint32_t>(activeTasks));
  } else {
    renderer->renderTasks(
        ownedTilesFb, camera, world, perFrameData, renderTaskIDs);
  }

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
}

void DistributedLoadBalancer::runRenderTasks(FrameBuffer *,
    Renderer *,
    Camera *,
    World *,
    const utility::ArrayView<uint32_t> &,
    void *)
{
  NOT_IMPLEMENTED;
}

} // namespace mpi
} // namespace ospray
