// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
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

#include "FPSCounter.h"

#include "ospcommon/common.h"

namespace ospray {

  FPSCounter::FPSCounter()
  {
    smooth_nom = 0.;
    smooth_den = 0.;
    frameStartTime = 0.;
  }

  void FPSCounter::startRender()
  {
    frameStartTime = ospcommon::getSysTime();
  }

  void FPSCounter::doneRender() {
    double seconds = ospcommon::getSysTime() - frameStartTime;
    smooth_nom = smooth_nom * 0.8f + seconds;
    smooth_den = smooth_den * 0.8f + 1.f;
  }
}// namespace ospray
