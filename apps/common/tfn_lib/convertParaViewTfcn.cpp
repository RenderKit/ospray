// ======================================================================== //
// Copyright 2017 Intel Corporation                                         //
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

#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include "json/json.h"
#include "tfn_lib.h"

const static std::string USAGE =
  "Usage: ./ospCvtParaViewTfcn <paraview_fcn.json> <out_fcn.tfn>\n"
  "----\n"
  "Converts the exported ParaView transfer functon JSON format to the\n"
  "    transfer function format used by OSPRay's sample apps\n";

using namespace ospcommon;

inline float cvtSrgb(const float x) {
  if (x <= 0.0031308) {
    return 12.92 * x;
  } else {
    return (1.055) * std::pow(x, 1.0 / 2.4) - 0.055;
  }
}

int main(int argc, char **argv) {
  if (argc < 3 || std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
    std::cout << USAGE;
    return 1;
  }

  std::ifstream paraViewFile(argv[1]);
  if (!paraViewFile.is_open()) {
    throw std::runtime_error("#ospCvtParaViewTfcn: Error - failed to open file " + std::string(argv[2]));
  }
  Json::Value paraViewFcn;
  paraViewFile >> paraViewFcn;
  if (paraViewFcn.isArray()) {
    std::cout << "#ospCvtParaViewTfcn: Found array of transfer functions, exporting the first one\n";
    paraViewFcn = paraViewFcn[0];
  }
  if (!paraViewFcn.isObject()) {
    throw std::runtime_error("#ospCvtParaViewTfcn: Error - no transfer function object to import!\n");
  }

  std::string tfcnName;
  if (paraViewFcn["Name"].type() == Json::stringValue) {
    tfcnName = paraViewFcn["Name"].asString();
    std::cout << "#ospCvtParaViewTfcn: Converting transfer function '" << tfcnName << "'\n";
  } else {
    throw std::runtime_error("#ospCvtParaViewTfcn: Error - failed to read transfer function name\n");
  }

  if (paraViewFcn["ColorSpace"].type() == Json::stringValue
      && paraViewFcn["ColorSpace"].asString() == "Diverging") {
    std::cout << "#ospCvtParaViewTfcn: WARNING: ParaView's diverging color space "
      << "interpolation is not supported, colors may be incorrect\n";
  }


  // Read the value, opacity pairs and ignore the strange extra 0.5, 0 entries
  std::cout << "#ospCvtParaViewTfcn: Reading value, opacity pairs\n";
  std::vector<vec2f> opacities;
  Json::Value pvOpacities = paraViewFcn["Points"];
  if (!pvOpacities.isArray()) {
    std::cout << "#ospCvtParaViewTfcn: No opacity data, setting default of linearly increasing [0, 1]\n";
    opacities.push_back(vec2f(0.f, 0.f));
    opacities.push_back(vec2f(1.f, 1.f));
  } else {
    // We the first 2 of every 4 values which are the (value, opacity) pair
    // followed by some random (0.5, 0) value pair that ParaView throws in there
    for (Json::Value::ArrayIndex i = 0; i < pvOpacities.size(); i += 4) {
      const float val = pvOpacities[i].asFloat();
      const float opacity = pvOpacities[i + 1].asFloat();
      opacities.push_back(vec2f(val, opacity));
    }
  }
  const float dataValueMin = opacities[0].x;
  const float dataValueMax = opacities.back().x;
  // Re-scale the opacity value entries into [0, 1]
  for (auto &v : opacities) {
    v.x = (v.x - dataValueMin) / (dataValueMax - dataValueMin);
  }

  // Read the (val, RGB) pairs
  std::cout << "#ospCvtParaViewTfcn: Reading value, r, g, b tuples\n";
  std::vector<vec4f> rgbPoints;
  Json::Value pvColors = paraViewFcn["RGBPoints"];
  if (!pvColors.isArray()) {
    throw std::runtime_error("#ospCvtParaViewTfcn: Error - failed to find value, r, g, b 'RGBPoints' array\n");
  }
  for (Json::Value::ArrayIndex i = 0; i < pvColors.size(); i += 4) {
    const float val = (pvColors[i].asFloat() - dataValueMin) / (dataValueMax - dataValueMin);
    const float r = pvColors[i + 1].asFloat();
    const float g = pvColors[i + 2].asFloat();
    const float b = pvColors[i + 3].asFloat();
    rgbPoints.push_back(vec4f(val, r, g, b));
  }

  // Sample the color values since the piecewise_linear transferfunction doesn't
  // allow for value, RGB pairs to be set and assumes the RGB values are spaced uniformly
  std::vector<vec3f> rgbSamples;
  const int N_SAMPLES = 256;
  size_t lo = 0;
  size_t hi = 1;
  for (int i = 0; i < N_SAMPLES; ++i) {
    const float x = float(i) / float(N_SAMPLES - 1);
    vec3f color(0);
    if (i == 0) {
      color = vec3f(rgbPoints[0].y, rgbPoints[0].z, rgbPoints[0].w);
    } else if (i == N_SAMPLES - 1) {
      color = vec3f(rgbPoints.back().y, rgbPoints.back().z, rgbPoints.back().w);
    } else {
      // If we're over this val, find the next one
      if (x > rgbPoints[lo].x) {
        for (size_t j = lo; j < rgbPoints.size() - 1; ++j) {
          if (x <= rgbPoints[j + 1].x) {
            lo = j;
            hi = j + 1;
            break;
          }
        }
      }
      const float delta = x - rgbPoints[lo].x;
      const float interval = rgbPoints[hi].x - rgbPoints[lo].x;
      if (delta == 0 || interval == 0) {
        color = vec3f(rgbPoints[lo].y, rgbPoints[lo].z, rgbPoints[lo].w);
      } else {
        vec3f loColor = vec3f(rgbPoints[lo].y, rgbPoints[lo].z, rgbPoints[lo].w);
        vec3f hiColor = vec3f(rgbPoints[hi].y, rgbPoints[hi].z, rgbPoints[hi].w);
        color = loColor + delta / interval * (hiColor - loColor);
      }
    }
    rgbSamples.push_back(color);
  }

  tfn::TransferFunction converted(tfcnName, rgbSamples, opacities, 0, 1.0, 0.5f);
  converted.save(argv[2]);

  return 0;
}

