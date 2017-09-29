// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "math/spectrum.ih"


// convolutions of: normalization * D65 * CIE_XYZ * xyz2sRGB

spectrum spectrum_sRGB_r = {
  0.051757f, 
  0.005973f, 
  -0.137685f,
  -0.269377f,
  -0.037963f,
  0.441357f,
  0.626572f,
  0.279564f
};

spectrum spectrum_sRGB_g = {
  -0.047952f,
  -0.033259f,
  0.116357f,
  0.371543f,
  0.425224f,
  0.201331f,
  -0.007122f,
  -0.025022f
};

spectrum spectrum_sRGB_b = {
  0.362019f,
  0.588662f,
  0.138681f,
  -0.016748f,
  -0.049860f,
  -0.033765f,
  -0.011765f,
  -0.002544f
};
