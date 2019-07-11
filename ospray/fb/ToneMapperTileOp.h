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

#include "ImageOp.h"

namespace ospray {

  /*! \brief Generic tone mapping operator approximating ACES by default. */
  struct OSPRAY_SDK_INTERFACE ToneMapperTileOp : public TileOp
  {
    void commit() override;

    std::unique_ptr<LiveImageOp> attach(FrameBufferView &fbView) override;

    std::string toString() const override;

    // Params for the tone mapping curve
    float a, b, c, d;
    bool acesColor;
    float exposure;
  };

  struct OSPRAY_SDK_INTERFACE LiveToneMapperTileOp : public LiveTileOp {
    LiveToneMapperTileOp(FrameBufferView &fbView,
                          void *ispcEquiv);

    ~LiveToneMapperTileOp() override;

    void process(Tile &t) override;

    void *ispcEquiv;
  };

} // ::ospray
