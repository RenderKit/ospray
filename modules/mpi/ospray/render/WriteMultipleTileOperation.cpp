// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "WriteMultipleTileOperation.h"
#include "../fb/DistributedFrameBuffer.h"
#include "fb/DistributedFrameBuffer_ispc.h"

namespace ospray {
using namespace rkcommon;

/* LiveTileOperation for image-parallel rendering, where the same image tile
 * could optionally be rendered by multiple ranks.
 */
struct LiveWriteMultipleTile : public LiveTileOperation
{
  LiveWriteMultipleTile(DistributedFrameBuffer *dfb,
      const vec2i &begin,
      size_t tileID,
      size_t ownerID,
      WriteMultipleTileOperation *parent);

  void newFrame() override;

  // accumulate into ACCUM and VARIANCE, and for last tile read-out
  void process(const ospray::Tile &tile) override;

 private:
  int maxAccumID = 0;
  size_t instances = 1;
  bool writeOnceTile = true;
  // defer accumulation to get correct variance estimate
  ospray::Tile bufferedTile;
  bool tileBuffered = false;
  WriteMultipleTileOperation *parent = nullptr;
  // serialize when multiple instances of this tile arrive at the same time
  std::mutex mutex;
};

LiveWriteMultipleTile::LiveWriteMultipleTile(DistributedFrameBuffer *dfb,
    const vec2i &begin,
    size_t tileID,
    size_t ownerID,
    WriteMultipleTileOperation *parent)
    : LiveTileOperation(dfb, begin, tileID, ownerID), parent(parent)
{}

void LiveWriteMultipleTile::newFrame()
{
  maxAccumID = 0;
  instances = parent->tileInstances[tileID];
  writeOnceTile = instances <= 1;
  tileBuffered = false;
}

void LiveWriteMultipleTile::process(const ospray::Tile &tile)
{
  if (writeOnceTile) {
    finished.region = tile.region;
    finished.fbSize = tile.fbSize;
    finished.rcp_fbSize = tile.rcp_fbSize;
    accumulate(tile);
    tileIsFinished();
    return;
  }

  bool done = false;
  if (tile.accumID == 0) {
    finished.region = tile.region;
    finished.fbSize = tile.fbSize;
    finished.rcp_fbSize = tile.rcp_fbSize;

    const auto bytes = tile.region.size().y * (TILE_SIZE * sizeof(float));
    memcpy(accum.z, tile.z, bytes);
    memcpy(finished.z, tile.z, bytes);
  }

  {
    std::lock_guard<std::mutex> lock(mutex);
    maxAccumID = std::max(maxAccumID, tile.accumID);
    if (!tileBuffered && (tile.accumID & 1) == 0) {
      memcpy(&bufferedTile, &tile, sizeof(ospray::Tile));
      tileBuffered = true;
    } else {
      ispc::DFB_accumulateTileSimple((const ispc::VaryingTile *)&tile,
          (ispc::VaryingTile *)&accum,
          (ispc::VaryingTile *)&variance);

      if (dfb->hasNormalBuf() || dfb->hasAlbedoBuf()) {
        ispc::DFB_accumulateAuxTile((const ispc::VaryingTile *)&tile,
            (ispc::Tile *)&finished,
            (ispc::VaryingTile *)&accum);
      }
    }
    done = --instances == 0;
  }

  if (done) {
    auto sz = tile.region.size();
    if ((maxAccumID & 1) == 0) {
      // if maxAccumID is even, variance buffer is one accumulated tile
      // short, which leads to vast over-estimation of variance; thus
      // estimate variance now, when accum buffer is also one (the buffered)
      // tile short
      const float prevErr = DFB_computeErrorForTile((ispc::vec2i &)sz,
          (ispc::VaryingTile *)&accum,
          (ispc::VaryingTile *)&variance,
          maxAccumID - 1);
      // use maxAccumID for correct normalization
      // this is OK, because both accumIDs are even
      bufferedTile.accumID = maxAccumID;
      accumulate(bufferedTile);
      error = prevErr;
    } else {
      // correct normalization is with maxAccumID, which is odd here
      bufferedTile.accumID = maxAccumID;
      // but original bufferedTile.accumID is always even and thus won't be
      // accumulated into variance buffer
      DFB_accumulateTile((const ispc::VaryingTile *)&bufferedTile,
          (ispc::VaryingTile *)&finished,
          (ispc::VaryingTile *)&accum,
          (ispc::VaryingTile *)&variance,
          dfb->hasAccumBuf(),
          // disable accumulation of variance
          false);
      if (dfb->hasNormalBuf() || dfb->hasAlbedoBuf()) {
        ispc::DFB_accumulateAuxTile((const ispc::VaryingTile *)&bufferedTile,
            (ispc::Tile *)&finished,
            (ispc::VaryingTile *)&accum);
      }
      // but still need to update the error
      error = DFB_computeErrorForTile((ispc::vec2i &)sz,
          (ispc::VaryingTile *)&accum,
          (ispc::VaryingTile *)&variance,
          maxAccumID);
    }

    tileIsFinished();
  }
}

void WriteMultipleTileOperation::attach(DistributedFrameBuffer *dfb)
{
  mpiGroup = mpicommon::worker.dup();
  tileInstances.resize(dfb->getTotalTiles(), 1);
}

std::shared_ptr<LiveTileOperation> WriteMultipleTileOperation::makeTile(
    DistributedFrameBuffer *dfb,
    const vec2i &tileBegin,
    size_t tileID,
    size_t ownerID)
{
  return std::make_shared<LiveWriteMultipleTile>(
      dfb, tileBegin, tileID, ownerID, this);
}

std::string WriteMultipleTileOperation::toString() const
{
  return "ospray::WriteMultipleTileOperation";
}

void WriteMultipleTileOperation::syncTileInstances()
{
  mpicommon::bcast(
      tileInstances.data(), tileInstances.size(), MPI_INT, 0, mpiGroup.comm)
      .wait();
}

} // namespace ospray
