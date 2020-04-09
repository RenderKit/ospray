// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../ImageOp.h"

namespace ospray {

/*! The debug TileOp just dumps the input tiles out as PPM files,
   with some optional filename prefix. It can also add a color to the
   tile which will be passed down the pipeline after it, for testing
   the tile pipeline itself.
   To combine the output tiles into a single
   image you can use ImageMagick's montage command:
   montage `ls *.ppm | sort -V` -geometry +0+0 -tile MxN out.jpg
   where M is the number of tiles along X, and N the number of tiles along Y
 */
struct OSPRAY_SDK_INTERFACE SaveTiles : public TileOp
{
  void commit() override;

  std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

  std::string toString() const override;

  std::string prefix;
  vec3f addColor;
};

struct OSPRAY_SDK_INTERFACE LiveSaveTiles : public LiveTileOp
{
  LiveSaveTiles(FrameBufferView &fbView,
      const std::string &prefix,
      const vec3f &addColor);

  void process(Tile &t) override;

  std::string prefix;
  vec3f addColor;
};

} // namespace ospray
