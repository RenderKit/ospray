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

#include <chrono>
#include <limits>
#include <map>
#include <utility>
#include <fstream>
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "common/Data.h"
#include "ospcommon/utility/getEnvVar.h"
// ospray
#include "DistributedRaycast.h"
#include "../../common/DistributedModel.h"
#include "../../MPIDistributedDevice.h"
#include "../MPILoadBalancer.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "../../common/Profiling.h"
#include "lights/Light.h"
#include "lights/AmbientLight.h"
// ispc exports
#include "DistributedRaycast_ispc.h"

namespace ospray {
  namespace mpi {
    using namespace std::chrono;

    static size_t frameNumber = 0;

    static bool DETAILED_LOGGING = false;

    struct RegionInfo
    {
      int currentRegion;
      bool computeVisibility;
      bool *regionVisible;

      RegionInfo()
        : currentRegion(0), computeVisibility(true), regionVisible(nullptr)
      {}
    };

    struct RegionScreenBounds
    {
      box2f bounds;
      // The max-depth of the box, for sorting the compositing order
      float depth;

      RegionScreenBounds()
        : bounds(empty), depth(-std::numeric_limits<float>::infinity())
      {}

      // Extend the screen-space bounds to include p.xy, and update
      // the min depth.
      void extend(const vec3f &p) {
        if (p.z < 0) {
          bounds = box2f(vec2f(0), vec2f(1));
        } else {
          bounds.extend(vec2f(p.x * sign(p.z), p.y * sign(p.z)));
          bounds.lower.x = clamp(bounds.lower.x);
          bounds.upper.x = clamp(bounds.upper.x);
          bounds.lower.y = clamp(bounds.lower.y);
          bounds.upper.y = clamp(bounds.upper.y);
        }
      }
    };

    // Project the region onto the screen. If the object is behind the camera
    // or off-screen, the region will be empty.
    RegionScreenBounds projectRegion(const box3f &bounds, const Camera *camera);

    DistributedRegion::DistributedRegion() : id(-1) {}
    DistributedRegion::DistributedRegion(box3f bounds, int id)
      : bounds(bounds), id(id)
    {}
    bool DistributedRegion::operator==(const ospray::mpi::DistributedRegion &b) const
    {
      return id == b.id;
    }
    bool DistributedRegion::operator<(const ospray::mpi::DistributedRegion &b) const
    {
      return id < b.id;
    }

    // DistributedRaycastRenderer definitions /////////////////////////////////

    DistributedRaycastRenderer::DistributedRaycastRenderer()
      : numAoSamples(0), shadowsEnabled(false), camera(nullptr)
    {
      ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);

      auto logging = utility::getEnvVar<std::string>("OSPRAY_DP_API_TRACING").value_or("0");
      DETAILED_LOGGING = std::stoi(logging) != 0;

      if (DETAILED_LOGGING) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        auto job_name = utility::getEnvVar<std::string>("OSPRAY_JOB_NAME").value_or("log");
        std::string statsLogFile = job_name + std::string("-rank")
          + std::to_string(rank) + ".txt";
        statsLog = ospcommon::make_unique<std::ofstream>(statsLogFile.c_str());
      }
    }

    DistributedRaycastRenderer::~DistributedRaycastRenderer()
    {
      if (DETAILED_LOGGING) {
        *statsLog << "\n" << std::flush;
      }
    }

    void DistributedRaycastRenderer::commit()
    {
      Renderer::commit();
      regions.clear();
      ghostRegions.clear();
      regionIEs.clear();
      ghostRegionIEs.clear();

      numAoSamples = getParam1i("aoSamples", 0);
      oneSidedLighting = getParam1i("oneSidedLighting", 1);
      shadowsEnabled = getParam1i("shadowsEnabled", 0);

      lightData = (Data*)getParamData("lights");
      lightIEs.clear();
      ambientLight = vec3f(0.f);

      if (lightData) {
        for (size_t i = 0; i < lightData->size(); ++i) {
          const Light* const light = ((Light**)lightData->data)[i];
          // Extract color from ambient lights, and don't include them in the
          // light list
          const AmbientLight* const ambient =
            dynamic_cast<const AmbientLight*>(light);
          if (ambient) {
            ambientLight += ambient->getRadiance();
          } else {
            lightIEs.push_back(light->getIE());
          }
        }
      } else {
        static WarnOnce warn("No lights provided to the renderer, "
            "making a default ambient light");
        ambientLight = vec3f(0.2);
      }

      camera = reinterpret_cast<PerspectiveCamera*>(getParamObject("camera"));
      if (!camera) {
        throw std::runtime_error("DistributedRaycastRender only supports "
                                 "PerspectiveCamera");
      }

      ManagedObject *models = getParamObject("model", nullptr);
      Data *modelsData = dynamic_cast<Data*>(models);
      if (modelsData) {
        ManagedObject **handles = reinterpret_cast<ManagedObject**>(modelsData->data);
        for (size_t i = 0; i < modelsData->numItems; ++i) {
          regions.push_back(dynamic_cast<DistributedModel*>(handles[i]));
        }
      } else if (model && dynamic_cast<DistributedModel*>(model)) {
        regions.push_back(dynamic_cast<DistributedModel*>(model));
      }

      ManagedObject *ghosts = getParamObject("ghostModel", nullptr);
      Data *ghostsData = dynamic_cast<Data*>(ghosts);
      if (ghostsData) {
        ManagedObject **handles = reinterpret_cast<ManagedObject**>(ghostsData->data);
        for (size_t i = 0; i < ghostsData->numItems; ++i) {
          ghostRegions.push_back(dynamic_cast<DistributedModel*>(handles[i]));
        }
      } else if (ghosts && dynamic_cast<DistributedModel*>(ghosts)) {
        ghostRegions.push_back(dynamic_cast<DistributedModel*>(ghosts));
      }

      std::transform(regions.begin(), regions.end(), std::back_inserter(regionIEs),
                     [](const DistributedModel *m) { return m->getIE(); });
      std::transform(ghostRegions.begin(), ghostRegions.end(), std::back_inserter(ghostRegionIEs),
                     [](const DistributedModel *m) { return m->getIE(); });

      exchangeModelBounds();

      void **lightsPtr = lightIEs.empty() ? nullptr : lightIEs.data();
      ispc::DistributedRaycastRenderer_set(getIE(),
                                           allRegions.data(),
                                           static_cast<int>(allRegions.size()),
                                           numAoSamples,
                                           oneSidedLighting,
                                           shadowsEnabled,
                                           (ispc::vec3f*)&ambientLight,
                                           lightIEs.size(),
                                           lightsPtr,
                                           regionIEs.data(),
                                           ghostRegionIEs.data());
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 channelFlags)
    {
      using namespace std::chrono;
      using namespace mpicommon;

      if (regions.empty()) {
        return renderNonDistrib(fb, channelFlags);
      }

      ProfilingPoint startRender;

      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
      dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);
      dfb->startNewFrame(errorThreshold);

      beginFrame(dfb);

      const size_t numRegions = allRegions.size();
      // Do a prepass and project each region's box to the screen to see
      // which tiles it touches, and at what depth.
      std::multimap<float, size_t> regionOrdering;
      std::vector<RegionScreenBounds> projectedRegions(numRegions, RegionScreenBounds());
      for (size_t i = 0; i < projectedRegions.size(); ++i) {
        if (!allRegions[i].bounds.empty()) {
          projectedRegions[i] = projectRegion(allRegions[i].bounds, camera);
          // Note that these bounds are very conservative, the bounds we compute
          // below are much tighter, and better. We just use the depth from the
          // projection to sort the tiles later
          projectedRegions[i].bounds.lower *= dfb->size;
          projectedRegions[i].bounds.upper *= dfb->size;
          regionOrdering.insert(std::make_pair(projectedRegions[i].depth, i));
#if 0
          if (mpicommon::globalRank() == 0) {
            std::cout << "region " << i << " projects too {"
              << projectedRegions[i].bounds << ", z = "
              << projectedRegions[i].depth << "}\n";
          }
#endif
        } else {
          projectedRegions[i] = RegionScreenBounds();
          projectedRegions[i].depth = std::numeric_limits<float>::infinity();
          regionOrdering.insert(std::make_pair(std::numeric_limits<float>::infinity(), i));
        }
      }

      // Compute the sort order for the regions
      std::vector<int> sortOrder(numRegions, 0);
      int depthIndex = 0;
      for (const auto &e : regionOrdering) {
        sortOrder[e.second] = depthIndex++;
      }
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << "sortorder: {";
        for (const auto &e : sortOrder) {
          std::cout << e << ", ";
        }
        std::cout << "}\n" << std::flush;
      }
#endif

      // Pre-compute the list of tiles that we actually need to render to,
      // so we can get a better thread assignment when doing the real work
      // TODO Will: is this going to help much? Going through all the rays
      // to do this pre-pass may be too expensive, and instead we want
      // to just do something coarser based on the projected regions
      std::vector<int> tilesForFrame;

      for (const auto &region : regions) {
        const auto &projection = projectedRegions[region->id];
#if 0
        std::cout << "region " << region->id << " projects to "
          << projection.bounds << "\n" << std::flush;
#endif
        if (projection.bounds.empty() || projection.depth < 0) {
          continue;
        }
        // TODO: Should we push back by a few pixels as well just in case
        // for the random sampling? May need to spill +/- a pixel? I'm not sure
        const vec2i minTile(projection.bounds.lower.x / TILE_SIZE,
                            projection.bounds.lower.y / TILE_SIZE);
        const vec2i numTiles = fb->getNumTiles();
        const vec2i maxTile(std::min(std::ceil(projection.bounds.upper.x / TILE_SIZE), float(numTiles.x)),
                            std::min(std::ceil(projection.bounds.upper.y / TILE_SIZE), float(numTiles.y)));

        tilesForFrame.reserve((maxTile.x - minTile.x) * (maxTile.y - minTile.y));
        for (int y = minTile.y; y < maxTile.y; ++y) {
          for (int x = minTile.x; x < maxTile.x; ++x) {
            // If we share ownership of this region but aren't responsible
            // for rendering it to this tile, don't render it.
            const size_t tileIndex = size_t(x) + size_t(y) * numTiles.x;
            const auto &owners = regionOwners[region->id];
            const size_t numRegionOwners = owners.size();
            const size_t ownerRank = std::distance(owners.begin(),
                                                   owners.find(globalRank()));
            // TODO: Can we do a better than round-robin over all tiles assignment here?
            // It could be that we end up not evenly dividing the workload.
            const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
            if (regionTileOwner) {
              tilesForFrame.push_back(tileIndex);
            }
          }
        }
      }

      // Be sure to include all the tiles that we're the owner for as well,
      // in some cases none of our data might project to them.
      for (int i = globalRank(); i < dfb->getTotalTiles(); i += numGlobalRanks()) {
        tilesForFrame.push_back(i);
      }

      std::sort(tilesForFrame.begin(), tilesForFrame.end());
      {
        // Filter out any duplicate tiles, e.g. we double-count the background
        // tile we have to render as the owner and our data projects to the
        // same tile.
        auto end = std::unique(tilesForFrame.begin(), tilesForFrame.end());
        tilesForFrame.erase(end, tilesForFrame.end());

#define PARTITION_OUT_FINISHED_TILES 1
#if PARTITION_OUT_FINISHED_TILES
        // Filter any tiles which are finished due to reaching the
        // error threshold. This should let TBB better allocate threads only
        // to tiles that actually need work.
        const uint32_t numTiles_x = static_cast<uint32_t>(fb->getNumTiles().x);
        end = std::partition(tilesForFrame.begin(), tilesForFrame.end(),
                             [&](const int &i){
                               const uint32_t tile_y = i / numTiles_x;
                               const uint32_t tile_x = i - tile_y*numTiles_x;
                               const vec2i tileID(tile_x, tile_y);
                               return dfb->tileError(tileID) > errorThreshold;
                             });
        tilesForFrame.erase(end, tilesForFrame.end());
#endif
      }

      tasking::parallel_for(tilesForFrame.size(), [&](size_t taskIndex) {
        const int tileIndex = tilesForFrame[taskIndex];

        const uint32_t numTiles_x = static_cast<uint32_t>(fb->getNumTiles().x);
        const uint32_t tile_y = tileIndex / numTiles_x;
        const uint32_t tile_x = tileIndex - tile_y*numTiles_x;
        const vec2i tileID(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileID);
        const bool tileOwner = (tileIndex % numGlobalRanks()) == globalRank();
        const int NUM_JOBS = (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB;

#if !PARTITION_OUT_FINISHED_TILES
        if (dfb->tileError(tileID) <= errorThreshold) {
          return;
        }
#endif

        Tile __aligned(64) bgtile(tileID, dfb->size, accumID);

        RegionInfo regionInfo;
        // The visibility entries are sorted by the region id, matching
        // the order of the allRegions vector.
        regionInfo.regionVisible = STACK_BUFFER(bool, numRegions);
        std::fill(regionInfo.regionVisible, regionInfo.regionVisible + numRegions, false);

        // The first renderTile doesn't actually do any rendering, and instead
        // just computes which tiles the region projects to.
        tasking::parallel_for(static_cast<size_t>(NUM_JOBS), [&](size_t tIdx) {
          renderTile(&regionInfo, bgtile, tIdx);
        });
        regionInfo.computeVisibility = false;

        // If we own the tile send the background color and the count of children for the
        // number of regions projecting to it that will be sent.
        if (tileOwner) {
          bgtile.generation = 0;
          bgtile.children = std::count(regionInfo.regionVisible,
                                       regionInfo.regionVisible + numRegions, true);
          bgtile.sortOrder = std::numeric_limits<int>::max();
          std::fill(bgtile.r, bgtile.r + TILE_SIZE * TILE_SIZE, bgColor.x);
          std::fill(bgtile.g, bgtile.g + TILE_SIZE * TILE_SIZE, bgColor.y);
          std::fill(bgtile.b, bgtile.b + TILE_SIZE * TILE_SIZE, bgColor.z);
          std::fill(bgtile.a, bgtile.a + TILE_SIZE * TILE_SIZE, bgColor.w);
          std::fill(bgtile.z, bgtile.z + TILE_SIZE * TILE_SIZE, std::numeric_limits<float>::infinity());

          fb->setTile(bgtile);
        }

        // Render our regions that project to this tile and ship them off
        // to the tile owner.
        std::vector<size_t> myVisibleRegions;
        myVisibleRegions.reserve(regions.size());
        for (size_t i = 0; i < regions.size(); ++i) {
          const auto &region = regions[i];
          if (regionInfo.regionVisible[region->id]) {
            myVisibleRegions.push_back(i);
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
          Tile __aligned(64) tile(tileID, dfb->size, accumID);
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
          const auto &region = regions[i];
          const auto &owners = regionOwners[region->id];
          const size_t numRegionOwners = owners.size();
          const size_t ownerRank = std::distance(owners.begin(),
                                                 owners.find(globalRank()));
          const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
          if (regionTileOwner) {
            RegionInfo localInfo;
            localInfo.regionVisible = regionInfo.regionVisible;
            localInfo.computeVisibility = false;
            localInfo.currentRegion = i;
            tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
              renderTile(&localInfo, tile, tIdx);
            });
            tile.sortOrder = sortOrder[region->id];
            fb->setTile(tile);
          }
#if PARALLEL_REGION_RENDERING
        });
#else
        }
#endif
      });

      auto endRender = high_resolution_clock::now();

      dfb->waitUntilFinished();
      endFrame(nullptr, channelFlags);

      ProfilingPoint endComposite;

      if (DETAILED_LOGGING && frameNumber > 5) {
        const std::array<int, 3> localTimes {
          duration_cast<milliseconds>(endRender - startRender.time).count(),
          duration_cast<milliseconds>(endComposite.time - endRender).count(),
          duration_cast<milliseconds>(endComposite.time - startRender.time).count()
        };
        std::array<int, 3> maxTimes = {0};
        std::array<int, 3> minTimes = {0};
        MPI_Reduce(localTimes.data(), maxTimes.data(), maxTimes.size(), MPI_INT,
            MPI_MAX, 0, world.comm);
        MPI_Reduce(localTimes.data(), minTimes.data(), minTimes.size(), MPI_INT,
            MPI_MIN, 0, world.comm);

        if (globalRank() == 0) {
          std::cout << "Frame: " << frameNumber << "\n"
            << "Max render time: " << maxTimes[0] << "ms\n"
            << "Max compositing wait: " << maxTimes[1] << "ms\n"
            << "Max total time: " << maxTimes[2] << "ms\n"

            << "Min render time: " << minTimes[0] << "ms\n"
            << "Min compositing wait: " << minTimes[1] << "ms\n"
            << "Min total time: " << minTimes[2] << "ms\n"
            << "Compositing overhead: " << minTimes[1] << "ms\n"
            << "============\n" << std::flush;
        }
        *statsLog << "-----\nFrame: " << frameNumber << "\n";
        char hostname[1024] = {0};
        gethostname(hostname, 1023);

        *statsLog << "Rank " << globalRank() << " on " << hostname << " times:\n"
          << "\tRendering: " << localTimes[0] << "ms\n"
          << "\tCompositing: " << localTimes[1] << "ms\n"
          << "\tTotal: " << localTimes[2] << "ms\n"
          << "\tTouched Tiles: " << tilesForFrame.size() << "\n";

        dfb->reportTimings(*statsLog);
        logProfilingData(*statsLog, startRender, endComposite);
        maml::logMessageTimings(*statsLog);
        *statsLog << "-----\n" << std::flush;
      }
      ++frameNumber;
      return dfb->endFrame(errorThreshold);
    }

    // TODO WILL: This is only for benchmarking the compositing using
    // the same rendering code path. Remove it once we're done! Or push
    // it behind some ifdefs!
    float DistributedRaycastRenderer::renderNonDistrib(FrameBuffer *fb,
                                                       const uint32 channelFlags)
    {
      using namespace mpicommon;
      ProfilingPoint startRender;

      beginFrame(fb);

      tasking::parallel_for(fb->getTotalTiles(), [&](int taskIndex) {
        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = taskIndex / numTiles_x;
        const size_t tile_x = taskIndex - tile_y*numTiles_x;
        const vec2i tileID(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileID);

        if (fb->tileError(tileID) <= errorThreshold) {
          return;
        }

        Tile __aligned(64) tile(tileID, fb->size, accumID);

        // We use the task of rendering the first region to also fill out the block visiblility list
        const int NUM_JOBS = (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB;
        tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
          renderTile(NULL, tile, tIdx);
        });

        fb->setTile(tile);
      });

      endFrame(nullptr, channelFlags);

      ProfilingPoint endRender;

      if (DETAILED_LOGGING && frameNumber > 5) {
        const std::array<int, 1> localTimes = {
          duration_cast<milliseconds>(endRender.time - startRender.time).count(),
        };
        std::array<int, 1> maxTimes = {0};
        std::array<int, 1> minTimes = {0};
        MPI_Reduce(localTimes.data(), maxTimes.data(), maxTimes.size(), MPI_INT,
            MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(localTimes.data(), minTimes.data(), minTimes.size(), MPI_INT,
            MPI_MIN, 0, MPI_COMM_WORLD);
        int rank, worldSize;
        MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        if (rank == 0) {
          std::cout << "Frame: " << frameNumber << "\n"
            << "Max render time: " << maxTimes[0] << "ms\n"
            << "Min render time: " << minTimes[0] << "ms\n"
            << "============\n" << std::flush;
        }

        *statsLog << "-----\nFrame: " << frameNumber << "\n";
        char hostname[1024] = {0};
        gethostname(hostname, 1023);
        *statsLog << "Rank " << rank << " on " << hostname << " times:\n"
          << "\tRendering: " << localTimes[0] << "ms\n";
        logProfilingData(*statsLog, startRender, endRender);
      }

      ++frameNumber;
      return fb->endFrame(errorThreshold);
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
    }

    void DistributedRaycastRenderer::exchangeModelBounds() {
      allRegions.clear();

      std::vector<DistributedRegion> myRegions;
      myRegions.reserve(regions.size());
      std::transform(regions.begin(), regions.end(),
                     std::back_inserter(myRegions),
                     [](const DistributedModel *m) {
                       return DistributedRegion(m->bounds, m->id);
                     });

      for (int i = 0; i < mpicommon::numGlobalRanks(); ++i) {
        if (i == mpicommon::globalRank()) {
          messaging::bcast(i, myRegions);
          std::copy(myRegions.begin(), myRegions.end(),
              std::back_inserter(allRegions));

          for (const auto &r : myRegions) {
            regionOwners[r.id].insert(i);
          }
        } else {
          std::vector<DistributedRegion> recv;
          messaging::bcast(i, recv);
          std::copy(recv.begin(), recv.end(),
              std::back_inserter(allRegions));

          for (const auto &r : recv) {
            regionOwners[r.id].insert(i);
          }
        }
      }

      // Prune down to unique regions sorted by ID.
      std::sort(allRegions.begin(), allRegions.end());
      auto end = std::unique(allRegions.begin(), allRegions.end());
      allRegions.erase(end, allRegions.end());

#ifndef __APPLE__
      if (logLevel() >= 3) {
        for (int i = 0; i < mpicommon::numGlobalRanks(); ++i) {
          if (i == mpicommon::globalRank()) {
            postStatusMsg(1) << "Rank " << mpicommon::globalRank()
              << ": All regions in world {";
            for (const auto &b : allRegions) {
              postStatusMsg(1) << "\t" << b << ",";
            }
            postStatusMsg(1) << "}\n";

            postStatusMsg(1) << "Ownership Information: {";
            for (const auto &r : regionOwners) {
              postStatusMsg(1) << "(" << r.first << ": [";
              for (const auto &i : r.second) {
                postStatusMsg(1) << i << ", ";
              }
              postStatusMsg(1) << "])";
            }
            postStatusMsg(1) << "\n" << std::flush;
          }
          mpicommon::world.barrier();
        }
      }
#endif
    }

    RegionScreenBounds projectRegion(const box3f &bounds, const Camera *camera)
    {
      RegionScreenBounds screen;

      // Do the bottom of the box
      vec3f pt = bounds.lower;
      ProjectedPoint proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      vec3f v = pt - camera->pos;
      screen.depth = dot(v, camera->dir);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif
      // TODO: Manage sign if it's behind camera?

      pt.x = bounds.upper.x;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      pt.y = bounds.upper.y;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      pt.x = bounds.lower.x;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      // Do the top of the box
      pt.y = bounds.lower.y;
      pt.z = bounds.upper.z;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      pt.x = bounds.upper.x;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      pt.y = bounds.upper.y;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
      }
#endif

      pt.x = bounds.lower.x;
      proj = camera->projectPoint(pt);
      screen.extend(proj.screenPos);
      v = pt - camera->pos;
      screen.depth = std::max(dot(v, camera->dir), screen.depth);
#if 0
      if (mpicommon::globalRank() == 0) {
        std::cout << pt << " projects to " << proj.screenPos
          << ", z = " << dot(v, camera->dir) << "\n";
        std::cout << std::flush;
      }
#endif

      return screen;
    }

    OSP_REGISTER_RENDERER(DistributedRaycastRenderer, mpi_raycast);

  } // ::ospray::mpi
} // ::ospray

std::ostream& operator<<(std::ostream &os, const ospray::mpi::DistributedRegion &d) {
  os << "DistributedRegion { " << d.bounds << ", id = " << d.id;
  return os;
}
bool operator==(const ospray::mpi::DistributedRegion &a,
                const ospray::mpi::DistributedRegion &b)
{
  return a.id == b.id;
}
bool operator<(const ospray::mpi::DistributedRegion &a,
               const ospray::mpi::DistributedRegion &b)
{
  return a.id < b.id;
}

