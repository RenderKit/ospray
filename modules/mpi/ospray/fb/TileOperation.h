// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>
#include "fb/TileShared.h"

namespace ospray {

struct DistributedFrameBuffer;
struct LiveTileOperation;

// TODO: Probably should all be under mpi namespace as well

/* Base class for tile operations which can be attached to a distributed
 * framebuffer. The attach method will be called multiple times on each
 * rank, to make an instance of the live tile operation for each tile owned
 * by the rank.
 *
 * The tileoperation will be kept associated with the specific renderFrame
 * call, and can track additional state as needed by the renderer.
 */
struct TileOperation
{
  virtual ~TileOperation() = default;

  /* The attach method is called when the tile operation is first attached
   * to the framebuffer.
   */
  virtual void attach(DistributedFrameBuffer *dfb);

  /* The makeTile method is called to make the live tile operation for each
   * live tile owned by the rank.
   */
  virtual std::unique_ptr<LiveTileOperation> makeTile(
      DistributedFrameBuffer *dfb,
      const vec2i &tileBegin,
      size_t tileID,
      size_t ownerID) = 0;

  virtual std::string toString() const = 0;
};

/* Base descriptor for a tile: its position in the frame buffer,
 * its ID and which rank actually owns the tile for running tile operations
 */
struct TileDesc
{
  TileDesc(const vec2i &begin, size_t tileID, size_t ownerID);

  virtual ~TileDesc() = default;

  /*! returns whether this tile is one of this particular
      node's tiles */
  virtual bool mine() const
  {
    return false;
  }

  vec2i begin;
  size_t tileID, ownerID;
};

/* Base class for a tile operation which the current rank is responsible for
 * executing. Includes scratch space for storing results computed by the
 * tile operation
 */
struct LiveTileOperation : public TileDesc
{
  LiveTileOperation(DistributedFrameBuffer *dfb,
      const vec2i &begin,
      size_t tileID,
      size_t ownerID);

  virtual ~LiveTileOperation() override = default;

  // Called at the beginning of each frame
  virtual void newFrame() = 0;

  bool mine() const override
  {
    return true;
  }

  /* Called for each Tile rendered locally or received over the
   * network for this image tile.
   */
  virtual void process(const ispc::Tile &tile) = 0;

  /* Utility method provided to accumulate the finished tile data from the
   * passed tile into the "finished" tile data, which will be passed on to any
   * ImageOps or written to the framebuffer.
   * TODO: Maybe just manage this on the DFB side?
   */
  void accumulate(const ispc::Tile &tile);

  /* Method to be called when the tile operation is finished and the computed
   * image written to the "finished" member of the struct
   */
  void tileIsFinished();

  DistributedFrameBuffer *dfb;

  // estimated variance of this tile
  float error{0.f};

  // TODO: dynamically allocate to save memory when no ACCUM or VARIANCE
  // even more TODO: Tile contains much more data (e.g. AUX), but using only
  // the color buffer here ==> much wasted memory
  // also holds accumulated normal&albedo
  ispc::Tile __aligned(64) accum;
  ispc::Tile __aligned(64) variance;

  /* To be able to give
     the 'postaccum' pixel op a readily normalized tile we have to
     create a local copy (the tile stores only the accum value,
     and we cannot change this) */
  // also holds normalized normal&albedo in AOS format
  ispc::Tile __aligned(64) finished;

  //! the rbga32-converted colors
  // TODO: dynamically allocate to save memory when only uint32 / I8 colors
  vec4f __aligned(64) color[TILE_SIZE * TILE_SIZE];
};

} // namespace ospray
