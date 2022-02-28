// Copyright 2009-2022 Intel Corporation
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
namespace staticLoadBalancer {
using namespace mpicommon;
using namespace rkcommon;

Distributed::Distributed() {}

Distributed::~Distributed()
{
  handle.free();
}
void Distributed::renderFrame(
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
  std::set<int> tilesForFrame;
  for (int i = workerRank(); i < dfb->getTotalTiles(); i += workerSize()) {
    const uint32_t tile_y = i / dfb->getNumTiles().x;
    const uint32_t tile_x = i - tile_y * dfb->getNumTiles().x;
    const vec2i tileID(tile_x, tile_y);
    // Skip tiles that have been rendered to satisfactory error level
    if (dfb->tileError(tileID) > renderer->errorThreshold) {
      tilesForFrame.insert(i);
    }
  }

  const vec2i numTiles = dfb->getNumTiles();
  const vec2i fbSize = dfb->getNumPixels();
  for (const auto &id : world->myRegionIds) {
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
        // Skip tiles that have been rendered to satisfactory error level
        if (dfb->tileError(vec2i(x, y)) <= renderer->errorThreshold) {
          continue;
        }

        const int tileIndex = x + y * numTiles.x;
        const auto &owners = world->regionOwners[id];
        const size_t numRegionOwners = owners.size();
        const size_t ownerRank =
            std::distance(owners.begin(), owners.find(workerRank()));
        // TODO: Can we do a better than round-robin over all tiles
        // assignment here? It could be that we end up not evenly dividing
        // the workload.
        const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
        if (regionTileOwner) {
          tilesForFrame.insert(tileIndex);
        }
      }
    }
  }
#ifdef ENABLE_PROFILING
  ProfilingPoint end = ProfilingPoint();
  std::cout << "Initial tile for frame determination "
            << elapsedTimeMs(start, end) << "ms\n";
#endif

#ifdef ENABLE_PROFILING
  start = ProfilingPoint();
#endif
  tasking::parallel_for(tilesForFrame.size(), [&](size_t taskId) {
    auto tileIter = tilesForFrame.begin();
    std::advance(tileIter, taskId);
    const int tileIndex = *tileIter;
    const uint32_t numTiles_x = static_cast<uint32_t>(dfb->getNumTiles().x);
    const uint32_t tile_y = tileIndex / numTiles_x;
    const uint32_t tile_x = tileIndex - tile_y * numTiles_x;
    const vec2i tileID(tile_x, tile_y);
    const int32 accumID = dfb->accumID(tileID);
    const bool tileOwner = (tileIndex % workerSize()) == workerRank();
    const int NUM_JOBS = (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB;

    const auto fbSize = dfb->getNumPixels();

    Tile __aligned(64) bgtile(tileID, fbSize, accumID);

    // The visibility entries are sorted by the region id, matching
    // the order of the allRegions vector.
    bool *regionVisible = STACK_BUFFER(bool, numRegions);
    std::fill(regionVisible, regionVisible + numRegions, false);

    // The first renderTile doesn't actually do any rendering, and instead
    // just computes which tiles the region projects to, giving us the
    // exact bounds of the region's projection onto the image
    tasking::parallel_for(static_cast<size_t>(NUM_JOBS), [&](size_t tIdx) {
      renderer->computeRegionVisibility(
          dfb, camera, world, regionVisible, perFrameData, bgtile, tIdx);
    });

    // If we own the tile send the background color and the count of
    // children for the number of regions projecting to it that will be
    // sent.
    if (tileOwner) {
      bgtile.sortOrder = std::numeric_limits<int32_t>::max();
      bgtile.generation = 0;
      bgtile.children = 0;
      // Note: not using std::count here as seems to not count properly in debug
      // builds
      for (size_t i = 0; i < numRegions; ++i) {
        if (regionVisible[i]) {
          ++bgtile.children;
        }
      }
      dfb->setTile(bgtile);
    }

    // Render our regions that project to this tile and ship them off
    // to the tile owner.
    std::vector<size_t> myVisibleRegions;
    myVisibleRegions.reserve(world->myRegionIds.size());
    for (const auto &rid : world->myRegionIds) {
      if (regionVisible[rid]) {
        myVisibleRegions.push_back(rid);
      }
    }
    // If none of our regions are visible, we're done
    if (myVisibleRegions.empty()) {
      return;
    }

    // TODO: Will it really be much benefit to run the regions in parallel
    // as well? We already are running in parallel on the tiles and the
    // pixels within the tiles, so adding another level may actually just
    // give us worse cache coherence.
#define PARALLEL_REGION_RENDERING 1
#if PARALLEL_REGION_RENDERING
    tasking::parallel_for(myVisibleRegions.size(), [&](size_t vid) {
      const size_t rid = myVisibleRegions[vid];
      Tile __aligned(64) tile(tileID, fbSize, accumID);
#else
      for (const size_t &rid : myVisibleRegions) {
        Tile &tile = bgtile;
#endif
      tile.generation = 1;
      tile.children = 0;
      // If we share ownership of this region but aren't responsible
      // for rendering it to this tile, don't render it.
      // Note that we do need to double check here, since we could have
      // multiple shared regions projecting to the same tile, and we
      // could be the region tile owner for only some of those
      const auto &owners = world->regionOwners[rid];
      const size_t numRegionOwners = owners.size();
      const size_t ownerRank =
          std::distance(owners.begin(), owners.find(workerRank()));
      const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
      if (regionTileOwner) {
        tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
          renderer->renderRegionToTile(dfb,
              camera,
              world,
              world->allRegions[rid],
              perFrameData,
              tile,
              tIdx);
        });
        // Unused
        // tile.sortOrder = sortOrder[rid];
        dfb->setTile(tile);
      }
#if PARALLEL_REGION_RENDERING
    });
#else
    }
#endif
  });
#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Local rendering for frame " << elapsedTimeMs(start, end)
            << "ms\n";
#endif

  dfb->waitUntilFinished();
  renderer->endFrame(dfb, perFrameData);

  dfb->endFrame(renderer->errorThreshold, camera);
} // end func

// ***************************************************************************
void Distributed::renderFrameReplicated(DistributedFrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    DistributedWorld *world)
{
  bool askedForWork = false;
  int enableStaticBalancer;
  auto OSPRAY_STATIC_BALANCER =
      utility::getEnvVar<int>("OSPRAY_STATIC_BALANCER");

  enableStaticBalancer = OSPRAY_STATIC_BALANCER.value_or(0);

  std::shared_ptr<TileOperation> tileOperation = nullptr;
  if (fb->getLastRenderer() != renderer) {
    tileOperation = std::make_shared<WriteMultipleTileOperation>();
    fb->setTileOperation(tileOperation, renderer);
  } else {
    tileOperation = fb->getTileOperation();
  }

#ifdef ENABLE_PROFILING
  ProfilingPoint start;
#endif
  fb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(fb, world);
#ifdef ENABLE_PROFILING
  ProfilingPoint end;
  std::cout << "Start new frame took: " << elapsedTimeMs(start, end) << "ms\n";
#endif

  const auto fbSize = fb->getNumPixels();

  const int ALLTASKS = fb->getTotalTiles();
  int NTASKS = ALLTASKS / workerSize();
  // NOTE(jda) - If all tiles do not divide evenly among all worker ranks
  //             (a.k.a. ALLTASKS / worker.size has a remainder), then
  //             some ranks will have one extra tile to do. Thus NTASKS
  //             is incremented if we are one of those ranks.
  if ((ALLTASKS % workerSize()) > workerRank())
    NTASKS++;

  // do not pass all tasks at once, this way if other ranks want to steal work,
  // they can
  int maxTasksPerRound = 20;
  int numRounds = std::max(NTASKS / maxTasksPerRound, 1);
  int tasksPerRound = NTASKS / numRounds;
  int remainTasks = NTASKS % numRounds;
  int terminatedTasks = 0;
  int minActiveTasks = (ALLTASKS / workerSize()) * 0.25;

  std::unique_ptr<DynamicLoadBalancer> dynamicLB =
      make_unique<DynamicLoadBalancer>(handle, ALLTASKS);

  mpicommon::barrier(fb->getMPIGroup().comm).wait();
  int totalActiveTasks = ALLTASKS;
  // push current work to workQueue
  for (int i = 0; i < numRounds; i++) {
    Work myWork;
    myWork.ntasks = tasksPerRound;
    myWork.offset = i * tasksPerRound;
    myWork.ownerRank = workerRank();
    dynamicLB->addWork(myWork);
  }

  // Extra round for any remainder tasks
  if (remainTasks > 0) {
    Work myWork;
    myWork.ntasks = remainTasks;
    myWork.offset = numRounds * tasksPerRound;
    myWork.ownerRank = workerRank();
    dynamicLB->addWork(myWork);
  }
#ifdef ENABLE_PROFILING
  start = ProfilingPoint();
#endif
  /* TODO WILL: This can dispatch back to LocalTiledLoadBalancer::renderTiles
   * to render the tiles instead of repeating this loop here ourselves.
   */

  while (0 < totalActiveTasks) {
    int currentTasks = 0;
    int offset = 0;
    int ownerRank;
    if (0 < dynamicLB->getWorkSize()) {
      Work currWorkItem = dynamicLB->getWorkItemFront();
      currentTasks = currWorkItem.ntasks;
      offset = currWorkItem.offset;
      ownerRank = currWorkItem.ownerRank;

      askedForWork = false;
    }

    if (0 < currentTasks) {
      tasking::parallel_for(currentTasks, [&](int taskIndex) {
        const size_t tileID = (taskIndex + offset) * workerSize() + ownerRank;
        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = tileID / numTiles_x;
        const size_t tile_x = tileID - tile_y * numTiles_x;
        const vec2i tileId(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileId);

        if (fb->tileError(tileId) <= renderer->errorThreshold)
          return;

#if TILE_SIZE > MAX_TILE_SIZE
        auto tilePtr = make_unique<Tile>(tileId, fbSize, accumID);
        auto &tile = *tilePtr;
#else
          Tile __aligned(64) tile(tileId, fbSize, accumID);
#endif

        if (!fb->frameCancelled()) {
          tasking::parallel_for(
              numJobs(renderer->spp, accumID), [&](size_t tid) {
                renderer->renderTile(
                    fb, camera, world, perFrameData, tile, tid);
              });
        }

        fb->setTile(tile);
      });
    }
    dynamicLB->sendTerm(currentTasks);
    terminatedTasks = terminatedTasks + currentTasks;

    if (0 < terminatedTasks)
      dynamicLB->updateActiveTasks(terminatedTasks);
    totalActiveTasks = dynamicLB->getActiveTasks();
    terminatedTasks = 0;

    // if the total active tasks over all workers is more than some min, request
    // work
    if (currentTasks <= 0 && minActiveTasks < totalActiveTasks && !askedForWork
        && !enableStaticBalancer) {
      dynamicLB->requestWork();
      askedForWork = true;
    }
  }
#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Render loop took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";

  start = ProfilingPoint();
#endif

  fb->waitUntilFinished();

#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "Wait finished took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";

  start = ProfilingPoint();
#endif

  renderer->endFrame(fb, perFrameData);
  fb->endFrame(renderer->errorThreshold, camera);

#ifdef ENABLE_PROFILING
  end = ProfilingPoint();
  std::cout << "End frame took: " << elapsedTimeMs(start, end)
            << "ms, CPU %: " << cpuUtilization(start, end) << "%\n";
#endif
}

// ***************************************************************************
std::string Distributed::toString() const
{
  return "ospray::mpi::staticLoadBalancer::Distributed";
}

void Distributed::setObjectHandle(ObjectHandle &handle_)
{
  handle = handle_;
}
void Distributed::renderTiles(FrameBuffer *,
    Renderer *,
    Camera *,
    World *,
    const utility::ArrayView<int> &,
    void *)
{
  NOT_IMPLEMENTED;
}

} // namespace staticLoadBalancer
} // namespace mpi
} // namespace ospray
