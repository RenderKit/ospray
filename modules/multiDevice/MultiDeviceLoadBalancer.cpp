#include "MultiDeviceLoadBalancer.h"
#include "MultiDevice.h"

#include <rkcommon/tasking/parallel_for.h>

namespace ospray {

MultiDeviceLoadBalancer::MultiDeviceLoadBalancer(
    const std::vector<std::shared_ptr<TiledLoadBalancer>> &loadBalancers)
    : loadBalancers(loadBalancers)

{}

void MultiDeviceLoadBalancer::renderFrame(api::MultiDeviceObject *framebuffer,
    api::MultiDeviceObject *renderer,
    api::MultiDeviceObject *camera,
    api::MultiDeviceObject *world)
{
  // At this level we will distribute the tiles among the subdevices
  // and assign them the tiles to render.
  FrameBuffer *fb0 = nullptr;
  for (size_t i = 0; i < loadBalancers.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)framebuffer->objects[i];
    if (i==0) {
      fb0 = fbi;
    }
    fbi->beginFrame();
  }

  // We need these after all subdevices have finished rendering, so hang
  // on to the per frame data pointer each subdevice's renderer returned
  std::vector<void *> perFrameDatas;
  perFrameDatas.resize(loadBalancers.size(), nullptr);

  renderTiles(framebuffer,
      renderer,
      camera,
      world,
      fb0->getTileIDs(),
      perFrameDatas);

  for (size_t i = 0; i < loadBalancers.size(); ++i) {
    Renderer *ri = (Renderer *)renderer->objects[i];
    FrameBuffer *fbi = (FrameBuffer *)framebuffer->objects[i];
    ri->endFrame(fbi, perFrameDatas[i]);
  }
  for (size_t i = 0; i < loadBalancers.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)framebuffer->objects[i];
    fbi->setCompletedEvent(OSP_WORLD_RENDERED);
  }

  // TODO For now everything is replicated, so just pick one,
  // but in the future we will have issues because this also executes
  // the image operations on the FB, which we may want to distribute
  // among the subdevices.
  {
    Renderer *r0 = (Renderer *)renderer->objects[0];
    Camera *c0 = (Camera *)camera->objects[0];
    fb0->endFrame(r0->errorThreshold, c0);
  }
  for (size_t i = 0; i < loadBalancers.size(); ++i) {
    FrameBuffer *fbi = (FrameBuffer *)framebuffer->objects[i];
    fbi->setCompletedEvent(OSP_FRAME_FINISHED);
  }
}

void MultiDeviceLoadBalancer::renderTiles(api::MultiDeviceObject *framebuffer,
    api::MultiDeviceObject *renderer,
    api::MultiDeviceObject *camera,
    api::MultiDeviceObject *world,
    const utility::ArrayView<int> &tileIDs,
    std::vector<void *> &perFrameDatas)
{
  //TODO:
  //investigate unrolling the foreach loadbalancers.size() and loadbalancers->renderTiles() loops
  //that should give tbb finer granularity to work with in its workstealing thing and delay load imbalances

  // Shuffle tile list into a round-robin ordering among the subdevices for better load balance.
  FrameBuffer *fb0 = (FrameBuffer *)framebuffer->objects[0];
  std::vector<int> allTileIDs;
  allTileIDs.reserve(tileIDs.size());
  for (unsigned int j = 0; j < loadBalancers.size(); ++j) {
    unsigned int tilesForSubdevice = tileIDs.size() / loadBalancers.size();
    // All tiles may not divide evenly among the subdevices
    if ((tileIDs.size() % loadBalancers.size()) > j) {
       tilesForSubdevice++;
    }
    for (int i = 0; i < static_cast<int>(tilesForSubdevice); ++i) {
      allTileIDs.push_back(i * loadBalancers.size() + j);
    }
  }

  // Render the whole frame distributing the tiles among the subdevices
  const int numTiles = tileIDs.size();
  tasking::parallel_for(loadBalancers.size(), [&](size_t subdeviceID) {
    // Tiles are assigned contiguously in chunks to the subdevices
    // from the tileIDs array but the contents of the array can
    // create a round robin or random assignment

    const int tilesForSubdevice = numTiles / loadBalancers.size();
    // All tiles may not divide evenly among the subdevices
    const int numExtraTiles = numTiles % loadBalancers.size();
    int tilesForThisDevice = tilesForSubdevice;
    int additionalTileOffset = 0;
    if (numExtraTiles != 0) {
      additionalTileOffset =
          std::min(static_cast<int>(subdeviceID), numExtraTiles);
      if (static_cast<int>(subdeviceID) < numExtraTiles) {
        tilesForThisDevice++;
      }
    }
    const int startTileIndex =
        subdeviceID * tilesForSubdevice + additionalTileOffset;
    utility::ArrayView<int> subdeviceTileIDs(
        &allTileIDs.at(startTileIndex), tilesForThisDevice);

    FrameBuffer *fb = (FrameBuffer *)framebuffer->objects[subdeviceID];
    Renderer *r = (Renderer *)renderer->objects[subdeviceID];
    Camera *c = (Camera *)camera->objects[subdeviceID];
    World *w = (World *)world->objects[subdeviceID];

    // TODO Note: potential issues with shared framebuffer but
    // per-subdevice renderers and worlds? Per-frame data issues?
    perFrameDatas[subdeviceID] = r->beginFrame(fb, w);
    loadBalancers[subdeviceID]->renderTiles(
        fb, r, c, w, subdeviceTileIDs, perFrameDatas[subdeviceID]);
  });

  //gather individual tiles down to fb 0.
  const auto fbSize = fb0->getNumPixels();

  tasking::parallel_for(loadBalancers.size(), [&](size_t subdeviceID) {
    if (subdeviceID == 0) return; //gather of fb0's tiles to fb0 is unecessary

    FrameBuffer *fbi = (FrameBuffer *)framebuffer->objects[subdeviceID];
    //get a hold of the pixels

    //TODO: formats taken from LocalFrameBuffer, are these always valid?
    const void *color = fbi->mapBuffer(OSP_FB_COLOR);
    int *colorI = (int*)color;
    float *colorF = (float*)color;
    float *depth = (float*)fbi->mapBuffer(OSP_FB_DEPTH);
    vec3f *normal = (vec3f*)fbi->mapBuffer(OSP_FB_NORMAL);
    vec3f *albedo = (vec3f*)fbi->mapBuffer(OSP_FB_ALBEDO);
    /*
    //TODO: ensure LocalFrameBuffer_accumulateTile leaves root with satellites' acc and var
    vec4f *accum = (vec4f*)fbi->mapBuffer(OSP_FB_ACCUM);
    vec4f *variance = (vec4f*)fbi->mapBuffer(OSP_FB_VARIANCE);
    */

    //get a hold of the list of tiles on this subdevice
    const int tilesForSubdevice = numTiles / loadBalancers.size();
    const int numExtraTiles = numTiles % loadBalancers.size();
    int tilesForThisDevice = tilesForSubdevice;
    int additionalTileOffset = 0;
    if (numExtraTiles != 0) {
      additionalTileOffset =
          std::min(static_cast<int>(subdeviceID), numExtraTiles);
      if (static_cast<int>(subdeviceID) < numExtraTiles) {
        tilesForThisDevice++;
      }
    }
    const int startTileIndex =
        subdeviceID * tilesForSubdevice + additionalTileOffset;
    utility::ArrayView<int> subdeviceTileIDs(
        &allTileIDs.at(startTileIndex), tilesForThisDevice);

    tasking::parallel_for(subdeviceTileIDs.size(), [&](size_t taskIndex) {
      const size_t numTiles_x = fbi->getNumTiles().x;
      const size_t tile_y = subdeviceTileIDs[taskIndex] / numTiles_x;
      const size_t tile_x = subdeviceTileIDs[taskIndex] - tile_y * numTiles_x;
      const vec2i tileID(tile_x, tile_y);
      const int32 accumID = 0;//fbi->accumID(tileID); this prevents re-accum across source and dest, which is good as dest's accum pixels are not valid (LocalFB.ispc:92)

  #if TILE_SIZE > MAX_TILE_SIZE
      auto tilePtr = make_unique<Tile>(tileID, fbSize, accumID);
      auto &tile = *tilePtr;
  #else
      Tile __aligned(64) tile(tileID, fbSize, accumID);
  #endif

#if 0
      const float showRanks = (float)subdeviceID/loadBalancers.size();
#else
      const float showRanks = 1.f;
#endif
      //TODO: think about means to access floats directly and avoid
      //the repeated conversions into, out of and back into RGB
      //TODO: these 'getTile' loops are unoptimized. Consider implementing in
      //ispc to accelerate.
      if (colorI && colorBufferFormat == OSP_FB_RGBA8) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            int *cS = colorI+(py*fb0->getNumPixels().x)+px;
            //RGBA8
            tile.a[cnt] = (float)((*cS&0xFF000000)>>24)/255.0;
            tile.b[cnt] = (float)((*cS&0x00FF0000)>>16)/255.0*showRanks;
            tile.g[cnt] = (float)((*cS&0x0000FF00)>>8)/255.0*showRanks;
            tile.r[cnt] = (float)((*cS&0x000000FF))/255.0*showRanks;
            cnt++;
          }
        }
      }
      if (colorI && colorBufferFormat == OSP_FB_SRGBA) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            int *cS = colorI+(py*fb0->getNumPixels().x)+px;
            //SRGBA - from LocalFB.ispc
            tile.a[cnt] = ((*cS&0xFF000000)>>24)/255.0;
            tile.b[cnt] = pow((float)((*cS&0x00FF0000)>>16)/255.0, 2.2f)*showRanks;
            tile.g[cnt] = pow((float)((*cS&0x0000FF00)>>8)/255.0, 2.2f)*showRanks;
            tile.r[cnt] = pow((float)((*cS&0x000000FF))/255.0, 2.2f)*showRanks;
            cnt++;
          }
        }
      }

      if (colorF && colorBufferFormat == OSP_FB_RGBA32F) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            float *cF = colorF+((py*fb0->getNumPixels().x)+px)*4;
            tile.r[cnt] = *(cF+0)*showRanks;
            tile.g[cnt] = *(cF+1)*showRanks;
            tile.b[cnt] = *(cF+2)*showRanks;
            tile.a[cnt] = *(cF+3);
            cnt++;
          }
        }
      }
      if (depth) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            tile.z[cnt] = *(depth+(py*fb0->getNumPixels().x)+px);
            cnt++;
          }
        }
      }
      if (normal) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            vec3f *cF = normal+(py*fb0->getNumPixels().x)+px;
            tile.nx[cnt] = cF->x;
            tile.ny[cnt] = cF->y;
            tile.nz[cnt] = cF->z;
            cnt++;
          }
        }
      }

      if (albedo) {
        int cnt = 0;
        for (int i=0; i < TILE_SIZE; ++i) {
          for (int j=0; j < TILE_SIZE; ++j) {
            int px = tile.region.lower.x+j;
            int py = tile.region.lower.y+i;
            if ((px >= fbSize.x)
                || (py >= fbSize.y)) {
                cnt++;
                continue;
            }
            vec3f *cF = albedo+(py*fb0->getNumPixels().x)+px;
            tile.nx[cnt] = cF->x;
            tile.ny[cnt] = cF->y;
            tile.nz[cnt] = cF->z;
            cnt++;
          }
        }
      }

      fb0->setTile(tile);

    });

    fbi->unmap(color);
    fbi->unmap(depth);
    fbi->unmap(normal);
    fbi->unmap(albedo);
    /*
    fbi->unmap(accum);
    fbi->unmap(variance);
    */

  });
}

std::string MultiDeviceLoadBalancer::toString() const
{
  return "ospray::MultiDeviceLoadBalancer";
}
} // namespace ospray
