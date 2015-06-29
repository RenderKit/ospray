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

// define ON and OFF to the flags that "ON"/"OFF" string values that
// cmake puts in below will make sense
#define ON 1
#define OFF 0

#define TILE_SIZE @OSPRAY_TILE_SIZE@

#define EXP_DISTRIBUTED_VOLUME @OSPRAY_EXP_DISTRIBUTED_VOLUME@
#define EXP_ALPHA_BLENDING @OSPRAY_EXP_ALPHA_BLENDING@
#define EXP_IMAGE_COMPOSITING @OSPRAY_EXP_COMPOSITING@

