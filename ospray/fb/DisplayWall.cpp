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

// ospray
#include "DisplayWall.h"
#include "FrameBuffer.h"

#ifdef OSPRAY_DISPLAYCLUSTER
// DisplayCluster
# include "dcStream.h"
#endif

namespace ospray {

  DisplayWallPO::Instance::Instance(DisplayWallPO *po, FrameBuffer *fb)
    : fb(fb), frameIndex(0), dcSocket(NULL), streamName("")
  {
#ifdef OSPRAY_DISPLAYCLUSTER
    const char *hostname = po->getParamString("hostname", "localhost");
    streamName = po->getParamString("streamName", "ospray");

    std::cerr << "connecting to host " << hostname << " for stream " << streamName << std::endl;

    // connect to DisplayCluster at given hostname.
    dcSocket = dcStreamConnect(hostname);

    if(!dcSocket)
      std::cerr << "could not connect to DisplayCluster at " << hostname << std::endl;
#else
    std::cout << "#osp:dw: display cluster support not compiled in" << std::endl;
#endif
  }

  void DisplayWallPO::Instance::beginFrame()
  {
    frameIndex++;

#ifdef OSPRAY_DISPLAYCLUSTER
    dcStreamSetFrameIndex(frameIndex);
#endif
  }

  void DisplayWallPO::Instance::postAccum(Tile &tile)
  {
#ifdef OSPRAY_DISPLAYCLUSTER
    if(!dcSocket)
      return;

    uint32 colorBuffer[TILE_SIZE*TILE_SIZE];

    size_t pixelID = 0;
    for (size_t iy=0;iy<TILE_SIZE;iy++)
      for (size_t ix=0;ix<TILE_SIZE;ix++, pixelID++) {
        vec4f col = vec4f(tile.r[pixelID],
                          tile.g[pixelID],
                          tile.b[pixelID],
                          tile.a[pixelID]);

        colorBuffer[pixelID] = cvt_uint32(col);
      }

    int sourceIndex = tile.region.lower.y*tile.fbSize.x + tile.region.lower.x;
    DcStreamParameters dcStreamParameters = dcStreamGenerateParameters(streamName.c_str(), sourceIndex, tile.region.lower.x, tile.fbSize.y-tile.region.upper.y, TILE_SIZE, TILE_SIZE, tile.fbSize.x, tile.fbSize.y);

    bool success = dcStreamSend(dcSocket, (unsigned char *)colorBuffer, tile.region.lower.x, tile.fbSize.y-tile.region.upper.y, tile.fbSize.x, 4*TILE_SIZE, tile.fbSize.y, RGBA, dcStreamParameters);

    if(!success) {
      std::cerr << "error sending tile to DisplayCluster, disconnecting." << std::endl;
      dcStreamDisconnect(dcSocket);
      dcSocket = NULL;
    }
#else 
    PRINT(tile.region);
#endif
  }


  OSP_REGISTER_PIXEL_OP(DisplayWallPO,display_wall);

}
