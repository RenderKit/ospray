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

// oidn
#include "OpenImageDenoise/oidn.h"
// ospray
#include "ospray/fb/ImageOp.h"
#include "ospray_module_denoiser_export.h"

namespace ospray {

  struct OSPRAY_MODULE_DENOISER_EXPORT DenoiseFrameOp : public FrameOp
  {
    DenoiseFrameOp();

    virtual ~DenoiseFrameOp() override;

    virtual std::unique_ptr<LiveImageOp> attach(
        FrameBufferView &fbView) override;

    std::string toString() const override;

   private:
    OIDNDevice device;
  };

}  // namespace ospray
