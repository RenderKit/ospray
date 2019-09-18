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
#include "DistributedFrameBuffer_TileMessages.h"
#include "../common/Messaging.h"

namespace ospray {

std::shared_ptr<mpicommon::Message> makeWriteTileMessage(
    const ospray::Tile &tile, bool hasAux)
{
  const size_t msgSize =
      hasAux ? sizeof(ospray::Tile) : sizeof(WriteTileMessage);
  auto msg = std::make_shared<mpicommon::Message>(msgSize + sizeof(int));
  const static int header = WORKER_WRITE_TILE;
  std::memcpy(msg->data, &header, sizeof(int));
  std::memcpy(msg->data + sizeof(int), &tile, msgSize);
  return msg;
}

void unpackWriteTileMessage(
    WriteTileMessage *msg, ospray::Tile &tile, bool hasAux)
{
  const size_t msgSize =
      hasAux ? sizeof(ospray::Tile) : sizeof(WriteTileMessage);
  std::memcpy(&tile, reinterpret_cast<char *>(msg) + sizeof(int), msgSize);
}

size_t masterMsgSize(
    OSPFrameBufferFormat fmt, bool hasDepth, bool hasNormal, bool hasAlbedo)
{
  size_t msgSize = 0;
  switch (fmt) {
  case OSP_FB_NONE:
    throw std::runtime_error(
        "Do not use per tile message for FB_NONE! (msgSize)");
  case OSP_FB_RGBA8:
  case OSP_FB_SRGBA:
    msgSize = sizeof(MasterTileMessage_RGBA_I8);
    break;
  case OSP_FB_RGBA32F:
    msgSize = sizeof(MasterTileMessage_RGBA_F32);
    break;
  }
  // Normal and Albedo also imply Depth
  if (hasDepth || hasNormal || hasAlbedo) {
    msgSize += sizeof(float) * TILE_SIZE * TILE_SIZE;
  }
  if (hasNormal || hasAlbedo) {
    msgSize += 2 * sizeof(vec3f) * TILE_SIZE * TILE_SIZE;
  }
  return msgSize;
}

MasterTileMessageBuilder::MasterTileMessageBuilder(OSPFrameBufferFormat fmt,
    bool hasDepth,
    bool hasNormal,
    bool hasAlbedo,
    vec2i coords,
    float error)
    : colorFormat(fmt),
      hasDepth(hasDepth),
      hasNormal(hasNormal),
      hasAlbedo(hasAlbedo)
{
  int command = 0;
  const size_t msgSize = masterMsgSize(fmt, hasDepth, hasNormal, hasAlbedo);
  switch (fmt) {
  case OSP_FB_NONE:
    throw std::runtime_error(
        "Do not use per tile message for FB_NONE! (msg ctor)");
  case OSP_FB_RGBA8:
  case OSP_FB_SRGBA:
    command = MASTER_WRITE_TILE_I8;
    pixelSize = sizeof(uint32);
    break;
  case OSP_FB_RGBA32F:
    command = MASTER_WRITE_TILE_F32;
    pixelSize = sizeof(vec4f);
    break;
  }
  // AUX also includes depth
  if (hasDepth) {
    command |= MASTER_TILE_HAS_DEPTH;
  }
  if (hasNormal || hasAlbedo) {
    command |= MASTER_TILE_HAS_AUX;
  }
  message = std::make_shared<mpicommon::Message>(msgSize);
  header = reinterpret_cast<MasterTileMessage_NONE *>(message->data);
  header->command = command;
  header->coords = coords;
  header->error = error;
}
void MasterTileMessageBuilder::setColor(const vec4f *color)
{
  if (colorFormat != OSP_FB_NONE) {
    const uint8_t *input = reinterpret_cast<const uint8_t *>(color);
    uint8_t *out = message->data + sizeof(MasterTileMessage_NONE);
    std::copy(input, input + pixelSize * TILE_SIZE * TILE_SIZE, out);
  }
}
void MasterTileMessageBuilder::setDepth(const float *depth)
{
  if (hasDepth) {
    float *out = reinterpret_cast<float *>(message->data
        + sizeof(MasterTileMessage_NONE) + pixelSize * TILE_SIZE * TILE_SIZE);
    std::copy(depth, depth + TILE_SIZE * TILE_SIZE, out);
  }
}
void MasterTileMessageBuilder::setNormal(const vec3f *normal)
{
  if (hasNormal) {
    auto out = reinterpret_cast<vec3f *>(message->data
        + sizeof(MasterTileMessage_NONE) + pixelSize * TILE_SIZE * TILE_SIZE
        + sizeof(float) * TILE_SIZE * TILE_SIZE);
    std::copy(normal, normal + TILE_SIZE * TILE_SIZE, out);
  }
}
void MasterTileMessageBuilder::setAlbedo(const vec3f *albedo)
{
  if (hasAlbedo) {
    auto out = reinterpret_cast<vec3f *>(message->data
        + sizeof(MasterTileMessage_NONE) + pixelSize * TILE_SIZE * TILE_SIZE
        + sizeof(float) * TILE_SIZE * TILE_SIZE
        + sizeof(vec3f) * TILE_SIZE * TILE_SIZE);
    std::copy(albedo, albedo + TILE_SIZE * TILE_SIZE, out);
  }
}

} // namespace ospray
