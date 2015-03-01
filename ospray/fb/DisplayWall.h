// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

/*! \file ospray/fb/DisplayWall.h 

  \brief EXPERIMENTAL initial steps to support for a "DisplayCluster" based display wall 
 */

// ospray
#include "ospray/fb/PixelOp.h"

namespace ospray {

  struct DisplayWallPO : public PixelOp {
    struct Instance : public PixelOp::Instance {
      FrameBuffer *fb;

      Instance(FrameBuffer *fb);
      virtual ~Instance() {}

      virtual void postAccum(Tile &tile) { PING; };
    };

    virtual Instance *createInstance(FrameBuffer *fb) { return new Instance(fb); };
  };

}
