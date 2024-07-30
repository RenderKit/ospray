// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#include "DistributedFrameBuffer_TileMessages.h"
#include "../common/Messaging.h"

namespace ospray {

std::shared_ptr<mpicommon::Message> makeWriteTileMessage(const ispc::Tile &tile)
{
  auto msg = std::make_shared<mpicommon::Message>(sizeof(WriteTileMessage));
  const static int header = WORKER_WRITE_TILE;
  std::memcpy(msg->data, &header, sizeof(int));
  std::memcpy(msg->data + sizeof(int), &tile, sizeof(ispc::Tile));
  return msg;
}

void unpackWriteTileMessage(WriteTileMessage *msg, ispc::Tile &tile)
{
  std::memcpy(
      &tile, reinterpret_cast<char *>(msg) + sizeof(int), sizeof(ispc::Tile));
}

MasterTileMessageBuilder::MasterTileMessageBuilder(vec2i coords, float error)
{
  const size_t msgSize = sizeof(MasterTileMessage_FB);
  message = std::make_shared<mpicommon::Message>(msgSize);
  header = reinterpret_cast<MasterTileMessage *>(message->data);
  header->command = MASTER_WRITE_TILE;
  header->coords = coords;
  header->error = error;
}

void MasterTileMessageBuilder::setColor(const vec4f *color)
{
  vec4f *out =
      reinterpret_cast<vec4f *>(message->data + sizeof(MasterTileMessage));
  std::copy(color, color + TILE_SIZE * TILE_SIZE, out);
}

void MasterTileMessageBuilder::setTile(const ispc::Tile *tile)
{
  ispc::Tile *out = reinterpret_cast<Tile *>(message->data
      + sizeof(MasterTileMessage) + TILE_SIZE * TILE_SIZE * sizeof(vec4f));
  std::copy(tile, tile + 1, out);
}

} // namespace ospray
