// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fstream>
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "common/Data.h"
#include "ospcommon/utility/getEnvVar.h"
// ospray
#include "DistributedRaycast.h"
#include "../../common/DistributedModel.h"
#include "../MPILoadBalancer.h"
#include "../../fb/DistributedFrameBuffer.h"
// ispc exports
#include "DistributedRaycast_ispc.h"

namespace ospray {
  namespace mpi {
    using namespace std::chrono;

    static rusage prevUsage = {0};
    static rusage curUsage = {0};
    static high_resolution_clock::time_point prevWall, curWall;
    static size_t frameNumber = 0;

    static bool DETAILED_LOGGING = false;

    bool startsWith(const std::string &a, const std::string &prefix) {
      if (a.size() < prefix.size()) {
        return false;
      }
      return std::equal(prefix.begin(), prefix.end(), a.begin());
    }

    void logProcessStatistics(std::ostream &os) {
      const double elapsedCpu = curUsage.ru_utime.tv_sec + curUsage.ru_stime.tv_sec
                                - (prevUsage.ru_utime.tv_sec + prevUsage.ru_stime.tv_sec)
                                + 1e-6f * (curUsage.ru_utime.tv_usec + curUsage.ru_stime.tv_usec
                                           - (prevUsage.ru_utime.tv_usec + prevUsage.ru_stime.tv_usec));

      const double elapsedWall = duration_cast<duration<double>>(curWall - prevWall).count();
      os << "\tCPU: " << elapsedCpu / elapsedWall * 100.0 << "%\n";


      std::ifstream procStatus("/proc/" + std::to_string(getpid()) + "/status");
      std::string line;
      const static std::vector<std::string> propPrefixes{
        "Threads", "Cpus_allowed_list", "VmSize", "VmRSS"
      };
      while (std::getline(procStatus, line)) {
        for (const auto &p : propPrefixes) {
          if (startsWith(line, p)) {
            os << "\t" << line << "\n";
            break;
          }
        }
      }
    }

    struct RegionInfo
    {
      int currentRegion;
      int currentGhostRegion;
      bool computeVisibility;
      bool *regionVisible;

      RegionInfo()
        : currentRegion(0), currentGhostRegion(0),
        computeVisibility(true), regionVisible(nullptr)
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

    // DistributedRaycastRenderer definitions /////////////////////////////////

    DistributedRaycastRenderer::DistributedRaycastRenderer()
      : numAoSamples(0), camera(nullptr)
    {
      ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);

#if 1
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
#endif
    }

    DistributedRaycastRenderer::~DistributedRaycastRenderer()
    {
#if 1
      if (DETAILED_LOGGING) {
        *statsLog << "\n" << std::flush;
      }
#endif
    }

    void DistributedRaycastRenderer::commit()
    {
      Renderer::commit();

      numAoSamples = getParam1i("aoSamples", 0);
      camera = reinterpret_cast<PerspectiveCamera*>(getParamObject("camera"));
      if (!camera) {
        throw std::runtime_error("DistributedRaycastRender only supports "
                                 "PerspectiveCamera");
      }

      if (!dynamic_cast<DistributedModel*>(model)) {
        throw std::runtime_error("DistributedRaycastRender must use a "
                                 "DistributedModel from the "
                                 "MPIDistributedDevice");
      }
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 channelFlags)
    {
      using namespace std::chrono;
      using namespace mpicommon;
      DistributedModel *distribModel = dynamic_cast<DistributedModel*>(model);
      if (!distribModel) {
        return renderNonDistrib(fb, channelFlags);
      }

      auto startRender = high_resolution_clock::now();
      getrusage(RUSAGE_SELF, &prevUsage);
      prevWall = high_resolution_clock::now();

      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
      dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);
      dfb->startNewFrame(errorThreshold);
      dfb->beginFrame();

      ispc::DistributedRaycastRenderer_setRegions(ispcEquivalent,
          distribModel->allRegions.data(),
          distribModel->ghostRegions.data(),
          distribModel->allRegions.size(),
          numAoSamples);

      const size_t numRegions = distribModel->allRegions.size();

      beginFrame(dfb);

      // Do a prepass and project each region's box to the screen to see
      // which tiles it touches, and at what depth.
      std::multimap<float, size_t> regionOrdering;
      std::vector<RegionScreenBounds> projectedRegions(numRegions, RegionScreenBounds());
      for (size_t i = 0; i < projectedRegions.size(); ++i) {
        projectedRegions[i] = projectRegion(distribModel->allRegions[i].bounds, camera);
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

      for (const auto &region : distribModel->myRegions) {
        const auto &projection = projectedRegions[region.id];
#if 0
        std::cout << "region " << region.id << " projects to "
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
            const auto &regionOwners = distribModel->regionOwners[region.id];
            const size_t numRegionOwners = regionOwners.size();
            const size_t ownerRank = std::distance(regionOwners.begin(),
                                                   regionOwners.find(globalRank()));
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
        auto end = std::unique(tilesForFrame.begin(), tilesForFrame.end());
        tilesForFrame.erase(end, tilesForFrame.end());
      }

      tasking::parallel_for(static_cast<int>(tilesForFrame.size()), [&](int taskIndex) {
        const int tileIndex = tilesForFrame[taskIndex];

        const size_t numTiles_x = fb->getNumTiles().x;
        const size_t tile_y = tileIndex / numTiles_x;
        const size_t tile_x = tileIndex - tile_y*numTiles_x;
        const vec2i tileID(tile_x, tile_y);
        const int32 accumID = fb->accumID(tileID);
        const bool tileOwner = (tileIndex % numGlobalRanks()) == globalRank();
        const int NUM_JOBS = (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB;

        if (dfb->tileError(tileID) <= errorThreshold) {
          return;
        }

        Tile __aligned(64) tile(tileID, dfb->size, accumID);

        RegionInfo regionInfo;
        // The visibility entries are sorted by the region id, matching
        // the order of the allRegions vector.
        regionInfo.regionVisible = STACK_BUFFER(bool, numRegions);
        std::fill(regionInfo.regionVisible, regionInfo.regionVisible + numRegions, false);

        // The first renderTile doesn't actually do any rendering, and instead
        // just computes which tiles the region projects to.
        tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
          renderTile(&regionInfo, tile, tIdx);
        });
        regionInfo.computeVisibility = false;

        // If we own the tile send the background color and the count of children for the
        // number of regions projecting to it that will be sent.
        if (tileOwner) {
          tile.generation = 0;
          tile.children = std::count(regionInfo.regionVisible,
              regionInfo.regionVisible + numRegions, true);
          tile.sortOrder = std::numeric_limits<int>::max();
          std::fill(tile.r, tile.r + TILE_SIZE * TILE_SIZE, bgColor.x);
          std::fill(tile.g, tile.g + TILE_SIZE * TILE_SIZE, bgColor.y);
          std::fill(tile.b, tile.b + TILE_SIZE * TILE_SIZE, bgColor.z);
          std::fill(tile.a, tile.a + TILE_SIZE * TILE_SIZE, bgColor.w);
          std::fill(tile.z, tile.z + TILE_SIZE * TILE_SIZE, std::numeric_limits<float>::infinity());

          fb->setTile(tile);
        }

        // Render our regions that project to this tile and ship them off
        // to the tile owner.
        tile.generation = 1;
        tile.children = 0;
        for (size_t i = 0; i < distribModel->myRegions.size(); ++i) {
          const auto &region = distribModel->myRegions[i];
          if (!regionInfo.regionVisible[region.id]) {
            continue;
          }

          // If we share ownership of this region but aren't responsible
          // for rendering it to this tile, don't render it.
          // Note that we do need to double check here, since we could have
          // multiple shared regions projecting to the same tile, and we
          // could be the region tile owner for only some of those
          const auto &regionOwners = distribModel->regionOwners[region.id];
          const size_t numRegionOwners = regionOwners.size();
          const size_t ownerRank = std::distance(regionOwners.begin(),
                                                 regionOwners.find(globalRank()));
          const bool regionTileOwner = (tileIndex % numRegionOwners) == ownerRank;
          if (regionTileOwner) {
            regionInfo.currentGhostRegion = i;
            regionInfo.currentRegion = region.id;
            tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
              renderTile(&regionInfo, tile, tIdx);
            });
            tile.sortOrder = sortOrder[region.id];

            fb->setTile(tile);
          }
        }
      });

      auto endRender = high_resolution_clock::now();

      dfb->waitUntilFinished();
      endFrame(nullptr, channelFlags);

      auto endComposite = high_resolution_clock::now();

      getrusage(RUSAGE_SELF, &curUsage);
      curWall = high_resolution_clock::now();

#if 1
      if (DETAILED_LOGGING && frameNumber > 5) {
        const std::array<int, 3> localTimes = {
          duration_cast<milliseconds>(endRender - startRender).count(),
          duration_cast<milliseconds>(endComposite - endRender).count(),
          duration_cast<milliseconds>(endComposite - startRender).count()
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
        logProcessStatistics(*statsLog);
        maml::logMessageTimings(*statsLog);
        *statsLog << "-----\n" << std::flush;
      }
#endif
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
      auto startRender = high_resolution_clock::now();
      getrusage(RUSAGE_SELF, &prevUsage);
      prevWall = high_resolution_clock::now();

      fb->beginFrame();

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

      auto endRender = high_resolution_clock::now();
      getrusage(RUSAGE_SELF, &curUsage);
      curWall = high_resolution_clock::now();

#if 1
      if (DETAILED_LOGGING && frameNumber > 5) {
        const std::array<int, 1> localTimes = {
          duration_cast<milliseconds>(endRender - startRender).count(),
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
        logProcessStatistics(*statsLog);
      }
#endif

      ++frameNumber;
      return fb->endFrame(errorThreshold);
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
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

