// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Messaging.h"
#include "fb/LocalFB.h"
#include "fb/TileShared.h"

namespace ospray {

/*! color buffer and depth buffer on master */
enum COMMANDTAG
{
  /*! command tag that identifies a CommLayer::message as a write
    tile command. this is a command using for sending a tile of
    new samples to another instance of the framebuffer (the one
    that actually owns that tile) for processing and 'writing' of
    that tile on that owner node. */
  WORKER_WRITE_TILE = 1 << 1,
  /*! command tag used for sending 'final' tiles from the tile
      owner to the master frame buffer. Note that we *do* send a
      message back to the master even in cases where the master
      does not actually care about the pixel data - we still have
      to let the master know when we're done. */
  MASTER_WRITE_TILE = 1 << 2,
  // abort rendering the current frame
  CANCEL_RENDERING = 1 << 3,
  // Worker updating us on tiles completed
  PROGRESS_MESSAGE = 1 << 4
};

struct TileMessage
{
  int command{-1};
};

struct ProgressMessage : public TileMessage
{
  uint64_t numCompleted;
  int frameID;
};

struct MasterTileMessage : public TileMessage
{
  vec2i coords;
  float error;
};

// message sent to the master when a tile is finished.
struct MasterTileMessage_FB : public MasterTileMessage
{
  vec4f color[TILE_SIZE * TILE_SIZE];
  ispc::Tile tile;
};

/*! message sent from one node's instance to another, to tell that
    instance to write that tile */
struct WriteTileMessage : public TileMessage
{
  ispc::Tile tile;
};

std::shared_ptr<mpicommon::Message> makeWriteTileMessage(
    const ispc::Tile &tile);

void unpackWriteTileMessage(WriteTileMessage *msg, ispc::Tile &tile);

/*! The message builder lets us abstractly fill messages of different
 * types, while keeping the underlying message structs POD so they're
 * easy to send around.
 */
class MasterTileMessageBuilder
{
  MasterTileMessage *header;

 public:
  std::shared_ptr<mpicommon::Message> message;

  MasterTileMessageBuilder(vec2i coords, float error);

  void setColor(const vec4f *color);
  void setTile(const ispc::Tile *);
};
} // namespace ospray
