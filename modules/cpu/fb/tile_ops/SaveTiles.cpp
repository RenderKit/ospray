// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "SaveTiles.h"
#include "rkcommon/utility/SaveImage.h"

namespace ospray {

void SaveTiles::commit()
{
  prefix = getParam<std::string>("prefix", "");
  addColor = getParam<vec3f>("addColor", vec3f(0.f));
}

std::unique_ptr<LiveImageOp> SaveTiles::attach(FrameBufferView &fbView)
{
  return rkcommon::make_unique<LiveSaveTiles>(fbView, prefix, addColor);
}

std::string SaveTiles::toString() const
{
  return "SaveTiles";
}

LiveSaveTiles::LiveSaveTiles(
    FrameBufferView &_fbView, const std::string &prefix, const vec3f &addColor)
    : LiveTileOp(_fbView), prefix(prefix), addColor(addColor)
{}

inline float convert_srgb(const float x)
{
  if (x < 0.0031308) {
    return 12.92 * x;
  } else {
    return 1.055 * std::pow(x, 1.f / 2.4f) - 0.055;
  }
}

void LiveSaveTiles::process(Tile &tile)
{
  const int tile_x = tile.region.lower.x / TILE_SIZE;
  // TODO WILL: Why does tile store the fbsize?
  const int tile_y = (tile.fbSize.y - tile.region.upper.y) / TILE_SIZE;
  const int tile_id = tile_x + tile_y * (tile.fbSize.x / TILE_SIZE);

  const uint32_t w = TILE_SIZE, h = TILE_SIZE;
  // Convert to SRGB8 color
  std::vector<uint8_t> data(w * h * 4, 0);
  size_t pixelID = 0;
  for (size_t iy = 0; iy < h; ++iy) {
    for (size_t ix = 0; ix < w; ++ix, ++pixelID) {
      if (!prefix.empty()) {
        data[pixelID * 4] =
            255.f * convert_srgb(clamp(tile.r[pixelID], 0.f, 1.f));
        data[pixelID * 4 + 1] =
            255.f * convert_srgb(clamp(tile.g[pixelID], 0.f, 1.f));
        data[pixelID * 4 + 2] =
            255.f * convert_srgb(clamp(tile.b[pixelID], 0.f, 1.f));
      }

      // We write out the tile that came as input, then apply the color to
      // pass down the pipeline
      tile.r[pixelID] += addColor.x;
      tile.g[pixelID] += addColor.y;
      tile.b[pixelID] += addColor.z;
    }
  }

  if (!prefix.empty()) {
    const std::string file = prefix + std::to_string(tile_id) + ".ppm";
    rkcommon::utility::writePPM(
        file, w, h, reinterpret_cast<uint32_t *>(data.data()));
  }
}

} // namespace ospray
