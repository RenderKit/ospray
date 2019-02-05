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

#include "PerspectiveCamera.h"

namespace ospray {
  namespace sg {

    PerspectiveCamera::PerspectiveCamera()
      : Camera("perspective")
    {
      createChild("aspect", "float", 1.f,
                      NodeFlags::required |
                      NodeFlags::valid_min_max).setMinMax(1e-31f, 1e31f);
      createChild("fovy", "float", 60.f,
                      NodeFlags::required |
                      NodeFlags::gui_slider).setMinMax(.1f, 180.f);
      createChild("apertureRadius", "float", 0.f,
                      NodeFlags::valid_min_max).setMinMax(0.f, 1e31f);
      createChild("focusDistance", "float", 1.f,
                      NodeFlags::valid_min_max).setMinMax(0.f, 1e31f);
    }

    OSP_REGISTER_SG_NODE(PerspectiveCamera);

  } // ::ospray::sg
} // ::ospray


