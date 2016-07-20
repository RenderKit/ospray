// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "fb/PixelOp.h"

// DisplayCluster
class DcSocket;

namespace ospray {

  /*! \brief a 'display wall' pixel op, that will send ready tiles to
      a display wall software such as displayCluster */
  struct DisplayWallPO : public PixelOp {
    struct Instance : public PixelOp::Instance {
      FrameBuffer *fb;
      int frameIndex;

      //! DisplayCluster socket.
      DcSocket *dcSocket;

      //! DisplayCluster stream name.
      std::string streamName;

      Instance(DisplayWallPO *po, FrameBuffer *fb);
      virtual ~Instance() {}

      virtual void beginFrame();

      virtual void postAccum(Tile &tile);
    };

    virtual PixelOp::Instance *createInstance(FrameBuffer *fb, PixelOp::Instance *prev) { PING; return new Instance(this, fb); };

    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::DisplayWallPO"; }
  };

}
