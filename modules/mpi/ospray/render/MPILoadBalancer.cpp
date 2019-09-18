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
#include <fstream>

// ours
#include "../common/Profiling.h"
#include "../fb/DistributedFrameBuffer.h"
#include "MPILoadBalancer.h"
// ospray
#include "ospray/render/Renderer.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/tasking/schedule.h"
#include "ospcommon/utility/getEnvVar.h"
// std
#include <algorithm>

namespace ospray {
namespace mpi {

using namespace mpicommon;

using namespace std::chrono;

static size_t frameNumber = 0;

static bool REPL_DETAILED_LOGGING = false;

namespace staticLoadBalancer {

// staticLoadBalancer::Master definitions ///////////////////////////////

static std::unique_ptr<std::ofstream> statsLog = nullptr;

void openStatsLog()
{
  auto logging =
      utility::getEnvVar<std::string>("OSPRAY_DP_API_TRACING").value_or("0");
  REPL_DETAILED_LOGGING = std::stoi(logging) != 0;

  if (REPL_DETAILED_LOGGING) {
    auto job_name =
        utility::getEnvVar<std::string>("OSPRAY_JOB_NAME").value_or("log");
    std::string statsLogFile =
        job_name + std::string("-rank") + std::to_string(worldRank()) + ".txt";
    statsLog = ospcommon::make_unique<std::ofstream>(statsLogFile.c_str());
  }
}

// TODO WILL: There is no master anymore, all are workers on the
// ospray side
Master::Master()
{
  openStatsLog();
}

float Master::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World * /*world*/)
{
  DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
  assert(dfb);

  ProfilingPoint startRender;

  dfb->startNewFrame(renderer->errorThreshold);
  dfb->beginFrame();
  dfb->closeCurrentFrame();

  /* the client will do its magic here, and the distributed
     frame buffer will be writing tiles here, without us doing
     anything ourselves */
  dfb->waitUntilFinished();

  ProfilingPoint endRender;

  if (REPL_DETAILED_LOGGING) {
    *statsLog << "-----\nFrame: " << frameNumber << "\n";
    char hostname[1024] = {0};
    gethostname(hostname, 1023);

    *statsLog << "Master rank " << worldRank() << " on " << hostname << "\n"
              << "Frame took: "
              << duration_cast<milliseconds>(endRender.time - startRender.time)
                     .count()
              << "ms\n";

    dfb->reportTimings(*statsLog);
    logProfilingData(*statsLog, startRender, endRender);
    maml::logMessageTimings(*statsLog);
    *statsLog << "-----\n" << std::flush;
  }
  ++frameNumber;

  dfb->endFrame(renderer->errorThreshold, camera);
  return dfb->getVariance();
}

std::string Master::toString() const
{
  return "ospray::mpi::staticLoadBalancer::Master";
}

// staticLoadBalancer::Slave definitions ////////////////////////////////

Slave::Slave()
{
  openStatsLog();
}

float Slave::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World *world)
{
  auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);

  ProfilingPoint startRender;

  dfb->startNewFrame(renderer->errorThreshold);
  // dfb->beginFrame(); is called by renderer->beginFrame:
  void *perFrameData = renderer->beginFrame(fb, world);

  const auto fbSize = dfb->getNumPixels();

  const int ALLTASKS = fb->getTotalTiles();
  int NTASKS = ALLTASKS / worldSize();

  // NOTE(jda) - If all tiles do not divide evenly among all worker ranks
  //             (a.k.a. ALLTASKS / worker.size has a remainder), then
  //             some ranks will have one extra tile to do. Thus NTASKS
  //             is incremented if we are one of those ranks.
  if ((ALLTASKS % worldSize()) > worldRank())
    NTASKS++;

  tasking::parallel_for(NTASKS, [&](int taskIndex) {
    const size_t tileID = taskIndex * worldSize() + worldRank();
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

    if (!dfb->frameCancelled()) {
      tasking::parallel_for(numJobs(renderer->spp, accumID), [&](size_t tid) {
        renderer->renderTile(fb, camera, world, perFrameData, tile, tid);
      });
    }

    fb->setTile(tile);
  });
  auto endRender = high_resolution_clock::now();

  dfb->waitUntilFinished();
  renderer->endFrame(dfb, perFrameData);

  ProfilingPoint endComposite;

  if (REPL_DETAILED_LOGGING) {
    *statsLog << "-----\nFrame: " << frameNumber << "\n";
    char hostname[1024] = {0};
    gethostname(hostname, 1023);

    *statsLog
        << "Worker rank " << worldRank() << " on " << hostname << "\n"
        << "Frame took: "
        << duration_cast<milliseconds>(endComposite.time - startRender.time)
               .count()
        << "ms\n"
        << "Local rendering took: "
        << duration_cast<milliseconds>(endRender - startRender.time).count()
        << "ms\n";

    dfb->reportTimings(*statsLog);
    logProfilingData(*statsLog, startRender, endComposite);
    maml::logMessageTimings(*statsLog);
    *statsLog << "-----\n" << std::flush;
  }
  ++frameNumber;

  dfb->endFrame(inf, camera);
  return dfb->getVariance();
}

std::string Slave::toString() const
{
  return "ospray::mpi::staticLoadBalancer::Slave";
}

} // namespace staticLoadBalancer

namespace dynamicLoadBalancer {

// dynamicLoadBalancer::Master definitions //////////////////////////////

// TODO WILL: There is no master anymore, all are workers on the
// ospray side
Master::Master(ObjectHandle handle, int _numPreAllocated)
    : MessageHandler(handle), numPreAllocated(_numPreAllocated)
{
  preferredTiles.resize(worldSize());
  workerNotified.resize(worldSize());

  // TODO numPreAllocated should be estimated/tuned automatically
}

void Master::incoming(const std::shared_ptr<mpicommon::Message> &msg)
{
  const int requester = *(int *)msg->data;
  scheduleTile(requester);
}

void Master::scheduleTile(const int worker)
{
  if (workerNotified[worker])
    return;

  // choose tile from preferred queue
  auto queue = preferredTiles.begin() + worker;

  // else look for largest non-empty queue
  if (queue->empty())
    queue = std::max_element(preferredTiles.begin(),
        preferredTiles.end(),
        [](TileVector a, TileVector b) { return a.size() < b.size(); });

  TileTask task;
  if (queue->empty()) {
    workerNotified[worker] = true;
    task.tilesExhausted = true;
    // If we told all the workers that we're out of tiles, then we're
    // done with this frame.
    const auto notNotified =
        std::find(workerNotified.begin(), workerNotified.end(), false);
    if (notNotified == workerNotified.end()) {
      dfb->closeCurrentFrame();
    }
  } else {
    task = queue->back();
    queue->pop_back();
    task.tilesExhausted = false;
  }

  auto answer = std::make_shared<mpicommon::Message>(&task, sizeof(task));
  mpi::messaging::sendTo(worker, myId, answer);
}

void Master::generateTileTasks(
    DistributedFrameBuffer *const dfb, const float errorThreshold)
{
  struct ActiveTile
  {
    float error;
    vec2i id;
  };
  std::vector<ActiveTile> activeTiles;
  TileTask task;
  const auto numTiles = dfb->getNumTiles();
  for (int y = 0, tileNr = 0; y < numTiles.y; y++) {
    for (int x = 0; x < numTiles.x; x++, tileNr++) {
      const auto tileId = vec2i(x, y);
      const auto tileError = dfb->tileError(tileId);
      if (tileError <= errorThreshold)
        continue;

      // remember active tiles
      activeTiles.push_back({tileError, tileId});

      task.tileId = tileId;
      task.accumId = dfb->accumID(tileId);

      auto nr = dfb->ownerIDFromTileID(tileNr);
      preferredTiles[nr].push_back(task);
    }
  }

  if (activeTiles.empty())
    return;

  // sort active tiles, highest error first
  std::sort(activeTiles.begin(),
      activeTiles.end(),
      [](ActiveTile a, ActiveTile b) { return a.error > b.error; });

  // TODO: estimate variance reduction to avoid duplicating tiles that are
  // just slightly above errorThreshold too often
  auto it = activeTiles.begin();
  const size_t tilesTotal = dfb->getTotalTiles();
  // loop over (active) tiles multiple times (instead of e.g. computing
  // instance count) to have maximum distance between duplicated tiles in
  // queue ==> higher chance that duplicated tiles do not arrive at the
  // same time at DFB and thus avoid the mutex in
  // WriteMultipleTile::process
  for (auto i = activeTiles.size(); i < tilesTotal; i++) {
    const auto tileId = it->id;
    task.tileId = tileId;
    task.accumId = dfb->accumID(tileId);
    const auto tileNr = tileId.y * numTiles.x + tileId.x;
    auto nr = dfb->ownerIDFromTileID(tileNr);
    preferredTiles[nr].push_back(task);

    if (++it == activeTiles.end())
      it = activeTiles.begin(); // start again from beginning
  }
}

float Master::renderFrame(
    FrameBuffer *fb, Renderer *renderer, Camera *camera, World * /*world*/)
{
  dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
  assert(dfb);

  for (size_t i = 0; i < workerNotified.size(); ++i)
    workerNotified[i] = false;

  generateTileTasks(dfb, renderer->errorThreshold);

  dfb->startNewFrame(renderer->errorThreshold);
  dfb->beginFrame();

  for (int tiles = 0; tiles < numPreAllocated; tiles++)
    for (int workerID = 0; workerID < worldSize(); workerID++)
      scheduleTile(workerID);

  dfb->waitUntilFinished();

  dfb->endFrame(renderer->errorThreshold, camera);
  return dfb->getVariance();
}

std::string Master::toString() const
{
  return "osp::mpi::dynamicLoadBalancer::Master";
}

// dynamicLoadBalancer::Slave definitions ////////////////////////////////

Slave::Slave(ObjectHandle handle) : MessageHandler(handle) {}

void Slave::incoming(const std::shared_ptr<mpicommon::Message> &msg)
{
  auto task = *(TileTask *)msg->data;

  mutex.lock();
  if (task.tilesExhausted)
    tilesAvailable = false;
  else
    tilesScheduled++;
  mutex.unlock();

  if (tilesAvailable)
    tasking::schedule([&, task] { tileTask(task); });
  else
    cv.notify_one();
}

float Slave::renderFrame(
    FrameBuffer *_fb, Renderer *_renderer, Camera *_camera, World *_world)
{
  renderer = _renderer;
  fb = _fb;
  camera = _camera;
  world = _world;

  auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);

  tilesAvailable = true;
  tilesScheduled = 0;

  dfb->startNewFrame(renderer->errorThreshold);
  // dfb->beginFrame(); is called by renderer->beginFrame:
  perFrameData = renderer->beginFrame(fb, world);
  frameActive = true;

  dfb->waitUntilFinished(); // swap with below?
  { // wait for render threads to finish
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&] { return tilesScheduled == 0 && !tilesAvailable; });
  }
  frameActive = false;

  renderer->endFrame(dfb, perFrameData);

  dfb->endFrame(inf, camera);
  return dfb->getVariance();
}

void Slave::tileTask(const TileTask &task)
{
  const auto fbSize = fb->getNumPixels();
#if TILE_SIZE > MAX_TILE_SIZE
  auto tilePtr = make_unique<Tile>(task.tileId, fbSize, task.accumId);
  auto &tile = *tilePtr;
#else
  Tile __aligned(64) tile(task.tileId, fbSize, task.accumId);
#endif

  while (!frameActive)
    ; // PRINT(frameActive); // XXX busy wait for valid perFrameData

  auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
  if (!dfb->frameCancelled()) {
    tasking::parallel_for(
        numJobs(renderer->spp, task.accumId), [&](size_t tid) {
          renderer->renderTile(fb, camera, world, perFrameData, tile, tid);
        });
  }

  if (tilesAvailable)
    requestTile(); // XXX here or after setTile?

  fb->setTile(tile);

  std::lock_guard<std::mutex> lock(mutex);
  if (--tilesScheduled == 0)
    cv.notify_one();
}

void Slave::requestTile()
{
  int requester = mpi::worldRank();
  auto msg =
      std::make_shared<mpicommon::Message>(&requester, sizeof(requester));
  mpi::messaging::sendTo(mpi::masterRank(), myId, msg);
}

std::string Slave::toString() const
{
  return "osp::mpi::dynamicLoadBalancer::Slave";
}

} // namespace dynamicLoadBalancer
} // namespace mpi
} // namespace ospray
