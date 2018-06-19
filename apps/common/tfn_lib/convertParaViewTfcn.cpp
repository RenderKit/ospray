// ======================================================================== //
// Copyright 2016-2018 Intel Corporation                                         //
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
#include <common/sg/transferFunction/TransferFunction.h>
#include <common/sg/SceneGraph.h>
#include <common/sg/transferFunction/TransferFunction.h>


const static std::string USAGE =
  "Usage: ./ospCvtParaViewTfcn <paraview_fcn.json> <out_fcn.tfn>\n"
  "----\n"
  "Converts the exported ParaView transfer functon JSON format to the\n"
  "    transfer function format used by OSPRay's sample apps\n";

using namespace ospcommon;
using namespace ospray;

inline float cvtSrgb(const float x) {
  if (x <= 0.0031308) {
    return 12.92 * x;
  } else {
    return (1.055) * std::pow(x, 1.0 / 2.4) - 0.055;
  }
}

int main(int argc, const char **argv) {
  if (argc < 3 || std::strcmp(argv[1], "-h") == 0 || std::strcmp(argv[1], "--help") == 0) {
    std::cout << USAGE;
    return 1;
  }

  ospInit(&argc, argv);
  auto tfNode = sg::createNode("tfImporter", "TransferFunction")->nodeAs<sg::TransferFunction>();
  tfNode->loadParaViewTF(std::string(argv[1]));

  std::vector<vec2f> opacities;
  std::vector<vec4f> rgbPoints;
  for (auto opacity : tfNode->child("opacityControlPoints").nodeAs<sg::DataVector2f>()->v)
    opacities.emplace_back(opacity);
  for (auto color : tfNode->child("colorControlPoints").nodeAs<sg::DataVector4f>()->v)
    rgbPoints.emplace_back(color);

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

  tfn::TransferFunction converted(tfNode->name(), rgbSamples, opacities, 0, 1.0, 0.5f);
  converted.save(argv[2]);

  return 0;
}

