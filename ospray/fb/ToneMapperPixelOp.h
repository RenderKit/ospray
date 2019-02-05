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

// ospray
#include "PixelOp.h"

namespace ospray {

  /*! \brief Generic tone mapping operator approximating ACES by default. */
  struct OSPRAY_SDK_INTERFACE ToneMapperPixelOp : public PixelOp
  {
    struct OSPRAY_SDK_INTERFACE Instance : public PixelOp::Instance
    {
      Instance(void* ispcInstance);

      virtual void postAccum(Tile& tile) override;
      virtual std::string toString() const override;

      void* ispcInstance;
    };

    ToneMapperPixelOp();
    virtual void commit() override;
    virtual std::string toString() const override;
    virtual PixelOp::Instance* createInstance(FrameBuffer* fb, PixelOp::Instance* prev) override;
  };

} // ::ospray
