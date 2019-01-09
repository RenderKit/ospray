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

#include "ospray/fb/PixelOp.h"

namespace ospray {
  // The debug PixelOp just dumps the rendered final tiles out as PPM files,
  // with some optional filename prefix. To combine them into a single
  // image you can use ImageMagick's montage command:
  // montage `ls *.ppm | sort -V` -geometry +0+0 -tile MxN out.jpg
  // where M is the number of tiles along X, and N the number of tiles along Y
  struct DebugPixelOp : public PixelOp
  {
    void commit() override;

    struct Instance : public PixelOp::Instance
    {
      Instance(const std::string &prefix);
      void postAccum(Tile &tile) override;
      std::string toString() const override;

      std::string prefix;
    };

    PixelOp::Instance* createInstance(FrameBuffer *fb,
                                      PixelOp::Instance *prev) override;

    std::string prefix;
  };
}

