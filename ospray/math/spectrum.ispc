// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
  0.0598548,
  -0.0234574,
  -0.220138,
  -0.238902,
  0.316327,
  0.738315,
  0.323302,
  0.0446981
};

spectrum spectrum_sRGB_g = {
 -0.0567346,
 -0.0160361,
 0.223861,
 0.531185,
 0.337221,
 0.0149718,
 -0.0296053,
 -0.00486239
};

spectrum spectrum_sRGB_b = {
 0.420693,
 0.616597,
 0.0796766,
 -0.0496266,
 -0.0473149,
 -0.0167536,
 -0.00295686,
 -0.000314818
};
