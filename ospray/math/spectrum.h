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

#define SPECTRUM_SAMPLES 8
#define SPECTRUM_FIRSTWL 430.f
#define SPECTRUM_SPACING 35.f
// ==> 430..675

#define SPECTRUM_AL_ETA {0.570, 0.686, 0.813, 0.952, 1.11, 1.29, 1.49, 1.73}
#define SPECTRUM_AL_K {5.21, 5.64, 6.05, 6.45, 6.85, 7.24, 7.61, 7.94}

#define RGB_AL_ETA {1.47f, 0.984f, 0.602f}
#define RGB_AL_K {7.64f, 6.55f, 5.36f}
// = ((eta-1)^2+k^2)/((eta+1)^2+k^2)
#define RGB_AL_COLOR {0.909f, 0.916f, 0.923f}
