// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/Profiling.h"
#include "DistributedLoadBalancer.h"
#include <algorithm>
#include <limits>
#include <map>
#include "../fb/DistributedFrameBuffer.h"
#include "WriteMultipleTileOperation.h"
#include "camera/PerspectiveCamera.h"
#include "common/MPICommon.h"
#include "distributed/DistributedRenderer.h"
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace mpi {
namespace staticLoadBalancer {
using namespace mpicommon;
using namespace rkcommon;

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

  if (!reinterpret_cast<PerspectiveCamera *>(camera)) {
    throw std::runtime_error(
        "DistributedRaycastRender only supports PerspectiveCamera");
  }

  if (dfb->getLastRenderer() != renderer) {
    dfb->setTileOperation(renderer->tileOperation(), renderer);
  }

  dfb->startNewFrame(renderer->errorThreshold);

  void *perFrameData = renderer->beginFrame(dfb, world);

  const auto fbSize = dfb->getNumPixels();
  const size_t numRegions = world->allRegions.size();

  // Do a prepass and project each region's box to the screen to see
  // which tiles it touches, and at what depth.
  std::multimap<float, size_t> regionOrdering;
  std::vector<RegionScreenBounds> projectedRegions(
      numRegions, RegionScreenBounds());
  for (size_t i = 0; i < projectedRegions.size(); ++i) {
    const auto &r = world->allRegions[i];
    auto &proj = projectedRegions[i];
    if (!r.bounds.empty()) {
      proj = r.project(camera);
      // Note that these bounds are very conservative, the bounds we
      // compute below are much tighter, and better. We just use the depth
      // from the projection to sort the tiles later
      proj.bounds.lower =
          max(proj.bounds.lower * fbSize - TILE_SIZE, vec2f(0.f));
      proj.bounds.upper =
          min(proj.bounds.upper * fbSize + TILE_SIZE, vec2f(fbSize - 1.f));
      regionOrdering.insert(std::make_pair(proj.depth, i));
    } else {
      proj = RegionScreenBounds();
      proj.depth = std::numeric_limits<float>::infinity();
      regionOrdering.insert(
          std::make_pair(std::numeric_limits<float>::infinity(), i));
    }
  }

  // Compute the sort order for the regions
  std::vector<int> sortOrder(numRegions, 0);
  int depthIndex = 0;
  for (const auto &e : regionOrdering) {
    sortOrder[e.second] = depthIndex++;
  }

  // Pre-compute the list of tiles that we actually need to render to,
  // so we can get a better thread assignment when doing the real work
  // TODO Will: is this going to help much? Going through all the rays
  // to do this pre-pass may be too expensive, and instead we want
  // to just do something coarser based on the projected regions
  std::vector<int> tilesForFrame;
  for (const auto &region : world->allRegions) {
    const auto &projection = projectedRegions[region.id];
    if (projection.bounds.empty() || projection.depth < 0) {
      continue;
    }
    // TODO: Should we push back by a few pixels as well just in case
    // for the random sampling? May need to spill +/- a pixel? I'm not
    // sure
    const vec2i minTile(projection.bounds.lower.x / TILE_SIZE,
        projection.bounds.lower.y / TILE_SIZE);
    const vec2i numTiles = dfb->getNumTiles();
    const vec2i maxTile(
        std::min(std::ceil(projection.bounds.upper.x / TILE_SIZE),
            float(numTiles.x)),
        std::min(std::ceil(projection.bounds.upper.y / TILE_SIZE),
            float(numTiles.y)));

    tilesForFrame.reserve((maxTile.x - minTile.x) * (maxTile.y - minTile.y));
    for (int y = minTile.y; y < maxTile.y; ++y) {
      for (int x = minTile.x; x < maxTile.x; ++x) {
        // If we share ownership of this region but aren't responsible
        // for rendering it to this tile, don't render it.
        const size_t tileIndex = size_t(x) + size_t(y) * numTiles.x;
        const auto &owners = world->regionOwners[region.id];
        const size_t numRegionOwners = owners.size();
        const size_t ownerRank =
            std::distance(owners.begin(), owners.find(workerRank()));
        // TODO: Can we do a better than round-robin over all tiles
        // assignment here? It could be that we end up not evenly dividing
        // the workload.
        const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
        if (regionTileOwner) {
          tilesForFrame.push_back(tileIndex);
        }
      }
    }
  }

  // Be sure to include all the tiles that we're the owner for as well,
  // in some cases none of our data might project to them.
  for (int i = workerRank(); i < dfb->getTotalTiles(); i += workerSize()) {
    tilesForFrame.push_back(i);
  }

  std::sort(tilesForFrame.begin(), tilesForFrame.end());
  {
    // Filter out any duplicate tiles, e.g. we double-count the background
    // tile we have to render as the owner and our data projects to the
    // same tile.
    auto end = std::unique(tilesForFrame.begin(), tilesForFrame.end());
    tilesForFrame.erase(end, tilesForFrame.end());

    // Filter any tiles which are finished due to reaching the
    // error threshold. This should let TBB better allocate threads only
    // to tiles that actually need work.
    end = std::partition(
        tilesForFrame.begin(), tilesForFrame.end(), [&](const int &i) {
          const uint32_t tile_y = i / dfb->getNumTiles().x;
          const uint32_t tile_x = i - tile_y * dfb->getNumTiles().x;
          const vec2i tileID(tile_x, tile_y);
          return dfb->tileError(tileID) > renderer->errorThreshold;
        });
    tilesForFrame.erase(end, tilesForFrame.end());
  }

  // Render the tiles we've got to do now
  tasking::parallel_for(tilesForFrame.size(), [&](size_t taskIndex) {
    const int tileIndex = tilesForFrame[taskIndex];

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
      bgtile.children =
          std::count(regionVisible, regionVisible + numRegions, true);
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

    // TODO: Will it really be much benefit to run the regions in parallel
    // as well? We already are running in parallel on the tiles and the
    // pixels within the tiles, so adding another level may actually just
    // give us worse cache coherence.
#define PARALLEL_REGION_RENDERING 1
#if PARALLEL_REGION_RENDERING
    tasking::parallel_for(myVisibleRegions.size(), [&](size_t vid) {
      const size_t i = myVisibleRegions[vid];
      Tile __aligned(64) tile(tileID, fbSize, accumID);
#else
      for (const size_t &i : myVisibleRegions) {
        Tile &tile = bgtile;
#endif
      tile.generation = 1;
      tile.children = 0;
      // If we share ownership of this region but aren't responsible
      // for rendering it to this tile, don't render it.
      // Note that we do need to double check here, since we could have
      // multiple shared regions projecting to the same tile, and we
      // could be the region tile owner for only some of those
      const auto &region = world->allRegions[i];
      const auto &owners = world->regionOwners[region.id];
      const size_t numRegionOwners = owners.size();
      const size_t ownerRank =
          std::distance(owners.begin(), owners.find(workerRank()));
      const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
      if (regionTileOwner) {
        tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
          renderer->renderRegionToTile(dfb,
              camera,
              world,
              world->allRegions[i],
              perFrameData,
              tile,
              tIdx);
        });
        tile.sortOrder = sortOrder[region.id];
        dfb->setTile(tile);
      }
#if PARALLEL_REGION_RENDERING
    });
#else
    }
#endif
  });

  dfb->waitUntilFinished();
  renderer->endFrame(dfb, perFrameData);

  dfb->endFrame(renderer->errorThreshold, camera);
}

void Distributed::renderFrameReplicated(DistributedFrameBuffer *fb,
    Renderer *renderer,
    Camera *camera,
    DistributedWorld *world)
{
  using namespace std::chrono;
  std::shared_ptr<TileOperation> tileOperation = nullptr;
  if (fb->getLastRenderer() != renderer) {
    tileOperation = std::make_shared<WriteMultipleTileOperation>();
    fb->setTileOperation(tileOperation, renderer);
  } else {
    tileOperation = fb->getTileOperation();
  }

  ProfilingPoint start;
  fb->startNewFrame(renderer->errorThreshold);
  void *perFrameData = renderer->beginFrame(fb, world);
  ProfilingPoint end;

#if 1
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

  start = ProfilingPoint();
  tasking::parallel_for(NTASKS, [&](int taskIndex) {
    const size_t tileID = taskIndex * workerSize() + workerRank();
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
      tasking::parallel_for(numJobs(renderer->spp, accumID), [&](size_t tid) {
        renderer->renderTile(fb, camera, world, perFrameData, tile, tid);
      });
    }

    fb->setTile(tile);
  });
  end = ProfilingPoint();
#if 1
  std::cout << "Render loop took: " << elapsedTimeMs(start, end) << "ms, CPU %: "
    << cpuUtilization(start, end) << "%\n";
#endif

  start = ProfilingPoint();
  fb->waitUntilFinished();
  end = ProfilingPoint();
#if 1
  std::cout << "Wait finished took: " << elapsedTimeMs(start, end) << "ms, CPU %: "
    << cpuUtilization(start, end) << "%\n";
#endif

  start = ProfilingPoint();
  renderer->endFrame(fb, perFrameData);
  fb->endFrame(renderer->errorThreshold, camera);
  end = ProfilingPoint();
#if 1
  std::cout << "End frame took: " << elapsedTimeMs(start, end) << "ms, CPU %: "
    << cpuUtilization(start, end) << "%\n";
#endif
}

std::string Distributed::toString() const
{
  return "ospray::mpi::staticLoadBalancer::Distributed";
}
} // namespace staticLoadBalancer
} // namespace mpi
} // namespace ospray
