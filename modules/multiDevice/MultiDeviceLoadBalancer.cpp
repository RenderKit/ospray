#include "MultiDeviceLoadBalancer.h"
#include <rkcommon/tasking/parallel_for.h>
// TODO Note: MultiDeviceObject should move to its own header
#include "MultiDevice.h"

namespace ospray {

MultiDeviceLoadBalancer::MultiDeviceLoadBalancer(
    const std::vector<std::shared_ptr<TiledLoadBalancer>> &loadBalancers)
    : loadBalancers(loadBalancers)

{}

void MultiDeviceLoadBalancer::renderFrame(FrameBuffer *fb,
    api::MultiDeviceObject *renderer,
    api::MultiDeviceObject *camera,
    api::MultiDeviceObject *world)
{
  // At this level we will distribute the tiles among the subdevices
  // and assign them the tiles to render.
  //
  // The multifuture needs to also run some function to internally decrement
  // the references of each thing and call the end frame for each
  // renderer and set completed parts
  fb->beginFrame();

  // We need these after all subdevices have finished rendering, so hang
  // on to the per frame data pointer each subdevice's renderer returned
  std::vector<void *> perFrameDatas;
  perFrameDatas.resize(loadBalancers.size(), nullptr);

  if (fb->getTotalTiles() != allTileIDs.size()) {
    initAllTileList(fb);
  }
  renderTiles(fb,
      renderer,
      camera,
      world,
      utility::ArrayView<int>(allTileIDs),
      perFrameDatas);

  for (size_t i = 0; i < loadBalancers.size(); ++i) {
    Renderer *r = (Renderer *)renderer->objects[i];
    r->endFrame(fb, perFrameDatas[i]);
  }
  fb->setCompletedEvent(OSP_WORLD_RENDERED);

  // TODO For now everything is replicated, so just pick one,
  // but in the future we will have issues because this also executes
  // the image operations on the FB, which we may want to distribute
  // among the subdevices.
  {
    Renderer *r = (Renderer *)renderer->objects[0];
    Camera *c = (Camera *)camera->objects[0];
    fb->endFrame(r->errorThreshold, c);
  }
  fb->setCompletedEvent(OSP_FRAME_FINISHED);
}

void MultiDeviceLoadBalancer::renderTiles(FrameBuffer *fb,
    api::MultiDeviceObject *renderer,
    api::MultiDeviceObject *camera,
    api::MultiDeviceObject *world,
    const utility::ArrayView<int> &tileIDs,
    std::vector<void *> &perFrameDatas)
{
  // Render the whole frame distributed the tiles among the subdevices
  const int numTiles = tileIDs.size();
  tasking::parallel_for(loadBalancers.size(), [&](size_t subdeviceID) {
    // Tiles are assigned contiguously in chunks to the subdevices
    // from the tileIDs array. The order of the tileIDs array can be
    // be used to create a round robin or random assignment
    const int tilesForSubdevice = numTiles / loadBalancers.size();
    const int numExtraTiles = numTiles % loadBalancers.size();

    // Tiles for this device may be increased by 1 if we're assigned extra tiles
    int tilesForThisDevice = tilesForSubdevice;
    int additionalTileOffset = 0;

    // All tiles may not divide evenly among the subdevices, extra tiles are
    // assigned to the lower ID subdevices, each taking one extra tile
    if (numExtraTiles != 0) {
      additionalTileOffset =
          std::min(static_cast<int>(subdeviceID), numExtraTiles);
      // Assign extra tiles to the subdevices in order
      if (subdeviceID < numExtraTiles) {
        tilesForThisDevice++;
      }
    }

    const int startTileIndex =
        subdeviceID * tilesForSubdevice + additionalTileOffset;
    utility::ArrayView<int> subdeviceTileIDs(
        &tileIDs.at(startTileIndex), tilesForThisDevice);

    Renderer *r = (Renderer *)renderer->objects[subdeviceID];
    Camera *c = (Camera *)camera->objects[subdeviceID];
    World *w = (World *)world->objects[subdeviceID];

    // TODO Note: potential issues with shared framebuffer but
    // per-subdevice renderers and worlds? Per-frame data issues?
    perFrameDatas[subdeviceID] = r->beginFrame(fb, w);
    loadBalancers[subdeviceID]->renderTiles(
        fb, r, c, w, subdeviceTileIDs, perFrameDatas[subdeviceID]);
  });
}

std::string MultiDeviceLoadBalancer::toString() const
{
  return "ospray::MultiDeviceLoadBalancer";
}

void MultiDeviceLoadBalancer::initAllTileList(const FrameBuffer *fb)
{
  // Create the tile list in a round-robin ordering among the subdevices
  allTileIDs.clear();
  allTileIDs.reserve(fb->getTotalTiles());
  for (int j = 0; j < loadBalancers.size(); ++j) {
    // Static round robin distribution of tiles among subdevices for now
    int tilesForSubdevice = fb->getTotalTiles() / loadBalancers.size();
    // All tiles may not divide evenly among the subdevices
    if ((fb->getTotalTiles() % loadBalancers.size()) > j) {
      tilesForSubdevice++;
    }

    for (int i = 0; i < tilesForSubdevice; ++i) {
      allTileIDs.push_back(i * loadBalancers.size() + j);
    }
  }
}
} // namespace ospray
