// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AlphaCompositeTileOperation.h"
#include <memory>
#include "../../fb/DistributedFrameBuffer.h"
#include "fb/DistributedFrameBuffer_ispc.h"

namespace ospray {

struct BufferedTile
{
  ispc::Tile tile;

  /*! determines order of this tile relative to other tiles.
    Tiles will get blended with the 'over' operator in
    increasing 'BufferedTile::sortOrder' value */
  float sortOrder;
};

/* LiveTileOperation for data-parallel or hybrid-parallel rendering, where
 * different (or partially shared) data is rendered on each rank
 */
struct LiveAlphaCompositeTile : public LiveTileOperation
{
  LiveAlphaCompositeTile(DistributedFrameBuffer *dfb,
      const vec2i &begin,
      size_t tileID,
      size_t ownerID);

  void newFrame() override;

  void process(const ispc::Tile &tile) override;

 private:
  std::vector<std::unique_ptr<BufferedTile>> bufferedTiles;
  int currentGeneration{0};
  int expectedInNextGeneration{0};
  int missingInCurrentGeneration{1};
  std::mutex mutex;

  void reportCompositingError(const vec2i &tile);
};

LiveAlphaCompositeTile::LiveAlphaCompositeTile(DistributedFrameBuffer *dfb,
    const vec2i &begin,
    size_t tileID,
    size_t ownerID)
    : LiveTileOperation(dfb, begin, tileID, ownerID)
{}

void LiveAlphaCompositeTile::newFrame()
{
  std::lock_guard<std::mutex> lock(mutex);
  currentGeneration = 0;
  expectedInNextGeneration = 0;
  missingInCurrentGeneration = 1;

  if (!bufferedTiles.empty()) {
    handleError(OSP_INVALID_OPERATION,
        std::to_string(mpicommon::workerRank())
            + " is starting with buffered tiles!");
  }
}

void LiveAlphaCompositeTile::process(const ispc::Tile &tile)
{
  std::lock_guard<std::mutex> lock(mutex);
  {
    auto addTile = rkcommon::make_unique<BufferedTile>();
    std::memcpy(&addTile->tile, &tile, sizeof(tile));

    bufferedTiles.push_back(std::move(addTile));
  }

  if (tile.generation == currentGeneration) {
    --missingInCurrentGeneration;
    expectedInNextGeneration += tile.children;

    if (missingInCurrentGeneration < 0) {
      reportCompositingError(tile.region.lower);
    }

    while (missingInCurrentGeneration == 0 && expectedInNextGeneration > 0) {
      currentGeneration++;
      missingInCurrentGeneration = expectedInNextGeneration;
      expectedInNextGeneration = 0;

      for (uint32_t i = 0; i < bufferedTiles.size(); i++) {
        const BufferedTile *bt = bufferedTiles[i].get();
        if (bt->tile.generation == currentGeneration) {
          --missingInCurrentGeneration;
          expectedInNextGeneration += bt->tile.children;
        }
        if (missingInCurrentGeneration < 0) {
          reportCompositingError(tile.region.lower);
        }
      }
    }
  }

  if (missingInCurrentGeneration < 0) {
    reportCompositingError(tile.region.lower);
  }

  if (missingInCurrentGeneration == 0) {
    // Sort for back-to-front blending
    // TODO: This is redundant with sortAndBlend. We should actually just do
    // this tile level sort, not sorting each pixel
    /*
  std::sort(bufferedTiles.begin(),
      bufferedTiles.end(),
      [](const std::unique_ptr<BufferedTile> &a,
          const std::unique_ptr<BufferedTile> &b) {
        return a->tile.sortOrder > b->tile.sortOrder;
      });
      */

    Tile **tileArray = STACK_BUFFER(Tile *, bufferedTiles.size());
    std::transform(bufferedTiles.begin(),
        bufferedTiles.end(),
        tileArray,
        [](std::unique_ptr<BufferedTile> &t) { return &t->tile; });

    ispc::DFB_sortAndBlendFragments(
        (ispc::VaryingTile **)tileArray, bufferedTiles.size());

    finished.region = tile.region;
    finished.fbSize = tile.fbSize;
    finished.rcp_fbSize = tile.rcp_fbSize;
    accumulate(bufferedTiles[0]->tile);
    bufferedTiles.clear();

    tileIsFinished();
  }
}

void LiveAlphaCompositeTile::reportCompositingError(const vec2i &tile)
{
  std::stringstream str;
  str << "negative missing on " << mpicommon::workerRank()
      << ", missingInCurrent = " << missingInCurrentGeneration
      << ", expectedInNext = " << expectedInNextGeneration
      << ", currentGeneration = " << currentGeneration << ", tile = " << tile;
  handleError(OSP_INVALID_OPERATION, str.str());
}

std::unique_ptr<LiveTileOperation> AlphaCompositeTileOperation::makeTile(
    DistributedFrameBuffer *dfb,
    const vec2i &tileBegin,
    size_t tileID,
    size_t ownerID)
{
  return make_unique<LiveAlphaCompositeTile>(dfb, tileBegin, tileID, ownerID);
}

std::string AlphaCompositeTileOperation::toString() const
{
  return "ospray::AlphaCompositeTileOperation";
}

} // namespace ospray
