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

#pragma once

#include "../common/Messaging.h"
#include "ospray/fb/LocalFB.h"

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
      message back ot the master even in cases where the master
      does not actually care about the pixel data - we still have
      to let the master know when we're done. */
  MASTER_WRITE_TILE_I8 = 1 << 2,
  MASTER_WRITE_TILE_F32 = 1 << 3,
  // Modifier to indicate the tile also has depth values
  MASTER_TILE_HAS_DEPTH = 1 << 4,
  // Indicates that the tile additionally also has normal and/or albedo values
  MASTER_TILE_HAS_AUX = 1 << 5,
  // abort rendering the current frame
  CANCEL_RENDERING = 1 << 6,
  // Worker updating us on tiles completed
  PROGRESS_MESSAGE = 1 << 7
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

/*! message sent to the master when a tile is finished. TODO:
    compress the color data */
template <typename ColorT>
struct MasterTileMessage_FB : public MasterTileMessage
{
  ColorT color[TILE_SIZE * TILE_SIZE];
};

template <typename ColorT>
struct MasterTileMessage_FB_Depth : public MasterTileMessage_FB<ColorT>
{
  float depth[TILE_SIZE * TILE_SIZE];
};

template <typename ColorT>
struct MasterTileMessage_FB_Depth_Aux
    : public MasterTileMessage_FB_Depth<ColorT>
{
  vec3f normal[TILE_SIZE * TILE_SIZE];
  vec3f albedo[TILE_SIZE * TILE_SIZE];
};

using MasterTileMessage_RGBA_I8 = MasterTileMessage_FB<uint32>;
using MasterTileMessage_RGBA_I8_Z = MasterTileMessage_FB_Depth<uint32>;
using MasterTileMessage_RGBA8_Z_AUX = MasterTileMessage_FB_Depth_Aux<uint32>;
using MasterTileMessage_RGBA_F32 = MasterTileMessage_FB<vec4f>;
using MasterTileMessage_RGBA_F32_Z = MasterTileMessage_FB_Depth<vec4f>;
using MasterTileMessage_RGBAF32_Z_AUX = MasterTileMessage_FB_Depth_Aux<vec4f>;
using MasterTileMessage_NONE = MasterTileMessage;

/*! message sent from one node's instance to another, to tell that
    instance to write that tile */
struct WriteTileMessage : public TileMessage
{
  region2i region; // screen region that this corresponds to
  vec2i fbSize; // total frame buffer size, for the camera
  vec2f rcp_fbSize;
  int32 generation;
  int32 children;
  int32 sortOrder;
  int32 accumID; //!< how often has been accumulated into this tile
  float pad[4]; //!< padding to match the ISPC-side layout
  float r[TILE_SIZE * TILE_SIZE]; // 'red' component
  float g[TILE_SIZE * TILE_SIZE]; // 'green' component
  float b[TILE_SIZE * TILE_SIZE]; // 'blue' component
  float a[TILE_SIZE * TILE_SIZE]; // 'alpha' component
  float z[TILE_SIZE * TILE_SIZE]; // 'depth' component
};

std::shared_ptr<mpicommon::Message> makeWriteTileMessage(
    const ospray::Tile &tile, bool hasAux);

void unpackWriteTileMessage(
    WriteTileMessage *msg, ospray::Tile &tile, bool hasAux);

size_t masterMsgSize(
    OSPFrameBufferFormat fmt, bool hasDepth, bool hasNormal, bool hasAlbedo);

/*! The message builder lets us abstractly fill messages of different
 * types, while keeping the underlying message structs POD so they're
 * easy to send around.
 */
class MasterTileMessageBuilder
{
  OSPFrameBufferFormat colorFormat;
  bool hasDepth;
  bool hasNormal;
  bool hasAlbedo;
  size_t pixelSize;
  MasterTileMessage_NONE *header;

 public:
  std::shared_ptr<mpicommon::Message> message;

  MasterTileMessageBuilder(OSPFrameBufferFormat fmt,
      bool hasDepth,
      bool hasNormal,
      bool hasAlbedo,
      vec2i coords,
      float error);
  void setColor(const vec4f *color);
  void setDepth(const float *depth);
  void setNormal(const vec3f *normal);
  void setAlbedo(const vec3f *albedo);
};
} // namespace ospray
