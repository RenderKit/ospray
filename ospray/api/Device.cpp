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
#include "Device.h"
#include "common/OSPCommon.h"
// embree
#include "embree2/rtcore.h"

namespace ospray {

  void error_handler(const RTCError code, const char *str);

  namespace api {

    Ref<Device> Device::current = nullptr;

    Device::Device() {
//      rtcSetErrorFunction(error_handler); need to call rtcInit first
    }

  } // ::ospray::api
} // ::ospray
