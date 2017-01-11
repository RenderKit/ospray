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

#pragma once

#include <vector>
#include "ospcommon/vec.h"
#include "ospcommon/FileName.h"

#ifdef _WIN32
  #ifdef ospray_tfn_EXPORTS
    #define OSPTFNLIB_INTERFACE __declspec(dllexport)
  #else
    #define OSPTFNLIB_INTERFACE __declspec(dllimport)
  #endif
#else
  #define OSPTFNLIB_INTERFACE
#endif

/* The transfer function file format used by the OSPRay sample apps is a
 * little endian binary format with the following layout:
 * uint32: magic number identifying the file
 * uint64: version number
 * uint64: length of the name of the transfer function (not including \0)
 * [char...]: name of the transfer function (without \0)
 * uint64: number of vec3f color values
 * uint64: numer of vec2f data value, opacity value pairs
 * float64: data value min
 * float64: data value max
 * float32: opacity scaling value, opacity values should be scaled
 *          by this factor
 * [ospcommon::vec3f...]: RGB values
 * [ospcommon::vec2f...]: data value, opacity value pairs
 */

namespace tfn {

struct OSPTFNLIB_INTERFACE TransferFunction {
  std::string name;
  std::vector<ospcommon::vec3f> rgbValues;
  std::vector<ospcommon::vec2f> opacityValues;
  double dataValueMin;
  double dataValueMax;
  float opacityScaling;

  // Load the transfer function data in the file
  TransferFunction(const ospcommon::FileName &fileName);
  // Construct a transfer function from some existing one
  TransferFunction(const std::string &name,
      const std::vector<ospcommon::vec3f> &rgbValues,
      const std::vector<ospcommon::vec2f> &opacityValues, const double dataValueMin,
      const double dataValueMax, const float opacityScaling);
  // Save the transfer function data to the file
  void save(const ospcommon::FileName &fileName) const;
};

}

