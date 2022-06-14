// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#include "DistributedFrameBuffer_TileMessages.h"
#include "../common/Messaging.h"

namespace ospray {

std::shared_ptr<mpicommon::Message> makeWriteTileMessage(
    const ispc::Tile &tile, bool hasAux)
{
  const size_t msgSize = hasAux ? sizeof(ispc::Tile) : sizeof(WriteTileMessage);
  auto msg = std::make_shared<mpicommon::Message>(msgSize + sizeof(int));
  const static int header = WORKER_WRITE_TILE;
  std::memcpy(msg->data, &header, sizeof(int));
  std::memcpy(msg->data + sizeof(int), &tile, msgSize);
  return msg;
}

void unpackWriteTileMessage(
    WriteTileMessage *msg, ispc::Tile &tile, bool hasAux)
{
  const size_t msgSize = hasAux ? sizeof(ispc::Tile) : sizeof(WriteTileMessage);
  std::memcpy(&tile, reinterpret_cast<char *>(msg) + sizeof(int), msgSize);
}

size_t masterMsgSize(OSPFrameBufferFormat fmt,
    bool hasDepth,
    bool hasNormal,
    bool hasAlbedo,
    bool hasIDBuffers)
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
  // If any ID buffer is present we allocate space for all of them to be sent
  if (hasIDBuffers) {
    msgSize += 3 * sizeof(uint32) * TILE_SIZE * TILE_SIZE;
  }
  return msgSize;
}

MasterTileMessageBuilder::MasterTileMessageBuilder(OSPFrameBufferFormat fmt,
    bool hasDepth,
    bool hasNormal,
    bool hasAlbedo,
    bool hasIDBuffers,
    vec2i coords,
    float error)
    : colorFormat(fmt),
      hasDepth(hasDepth),
      hasNormal(hasNormal),
      hasAlbedo(hasAlbedo),
      hasIDBuffers(hasIDBuffers)
{
  int command = 0;
  const size_t msgSize =
      masterMsgSize(fmt, hasDepth, hasNormal, hasAlbedo, hasIDBuffers);
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
  if (hasDepth || hasNormal || hasAlbedo) {
    command |= MASTER_TILE_HAS_DEPTH;
  }
  if (hasNormal || hasAlbedo) {
    command |= MASTER_TILE_HAS_AUX;
  }
  if (hasIDBuffers) {
    command |= MASTER_TILE_HAS_ID;
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
    float *out = reinterpret_cast<float *>(
        message->data + sizeof(MasterTileMessage_NONE) + colorBufferSize());
    std::copy(depth, depth + TILE_SIZE * TILE_SIZE, out);
  }
}

void MasterTileMessageBuilder::setNormal(const vec3f *normal)
{
  if (hasNormal) {
    vec3f *out =
        reinterpret_cast<vec3f *>(message->data + sizeof(MasterTileMessage_NONE)
            + colorBufferSize() + depthBufferSize());
    std::copy(normal, normal + TILE_SIZE * TILE_SIZE, out);
  }
}

void MasterTileMessageBuilder::setAlbedo(const vec3f *albedo)
{
  if (hasAlbedo) {
    vec3f *out =
        reinterpret_cast<vec3f *>(message->data + sizeof(MasterTileMessage_NONE)
            + colorBufferSize() + depthBufferSize() + normalBufferSize());
    std::copy(albedo, albedo + TILE_SIZE * TILE_SIZE, out);
  }
}

void MasterTileMessageBuilder::setPrimitiveID(const uint32 *primID)
{
  if (hasIDBuffers) {
    size_t offset = sizeof(MasterTileMessage_NONE) + colorBufferSize()
        + depthBufferSize() + normalBufferSize() + albedoBufferSize();
    uint32 *out = reinterpret_cast<uint32 *>(message->data + offset);
    std::copy(primID, primID + TILE_SIZE * TILE_SIZE, out);
  }
}

void MasterTileMessageBuilder::setObjectID(const uint32 *geomID)
{
  if (hasIDBuffers) {
    size_t offset = sizeof(MasterTileMessage_NONE) + colorBufferSize()
        + depthBufferSize() + normalBufferSize() + albedoBufferSize()
        + sizeof(uint32) * TILE_SIZE * TILE_SIZE;
    uint32 *out = reinterpret_cast<uint32 *>(message->data + offset);
    std::copy(geomID, geomID + TILE_SIZE * TILE_SIZE, out);
  }
}

void MasterTileMessageBuilder::setInstanceID(const uint32 *instID)
{
  if (hasIDBuffers) {
    size_t offset = sizeof(MasterTileMessage_NONE) + colorBufferSize()
        + depthBufferSize() + normalBufferSize() + albedoBufferSize()
        + 2 * sizeof(uint32) * TILE_SIZE * TILE_SIZE;
    uint32 *out = reinterpret_cast<uint32 *>(message->data + offset);
    std::copy(instID, instID + TILE_SIZE * TILE_SIZE, out);
  }
}

size_t MasterTileMessageBuilder::colorBufferSize() const
{
  return pixelSize * TILE_SIZE * TILE_SIZE;
}

size_t MasterTileMessageBuilder::depthBufferSize() const
{
  return hasDepth || hasNormal || hasAlbedo
      ? sizeof(float) * TILE_SIZE * TILE_SIZE
      : 0;
}

size_t MasterTileMessageBuilder::normalBufferSize() const
{
  return (hasNormal || hasAlbedo) ? sizeof(vec3f) * TILE_SIZE * TILE_SIZE : 0;
}

size_t MasterTileMessageBuilder::albedoBufferSize() const
{
  return (hasNormal || hasAlbedo) ? sizeof(vec3f) * TILE_SIZE * TILE_SIZE : 0;
}

size_t MasterTileMessageBuilder::idBufferSize() const
{
  return hasIDBuffers ? 3 * sizeof(uint32) * TILE_SIZE * TILE_SIZE : 0;
}

} // namespace ospray
