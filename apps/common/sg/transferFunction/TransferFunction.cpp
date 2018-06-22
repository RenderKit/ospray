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

#include "TransferFunction.h"

#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include "tfn_lib/tfn_lib.h"
#include "tfn_lib/json/json.h"

namespace ospray {
  namespace sg {

    TransferFunction::TransferFunction()
    {
      setValue((OSPTransferFunction)nullptr);
      createChild("valueRange", "vec2f", vec2f(0.f, 1.f));
      createChild("numSamples", "int", 256);

      auto colors = createChild("colors",
                                "DataVector3f").nodeAs<DataVector3f>();

      // 'Jet' transfer function
      colors->v.emplace_back(0       , 0, 0.562493);
      colors->v.emplace_back(0       , 0, 1       );
      colors->v.emplace_back(0       , 1, 1       );
      colors->v.emplace_back(0.500008, 1, 0.500008);
      colors->v.emplace_back(1       , 1, 0       );
      colors->v.emplace_back(1       , 0, 0       );
      colors->v.emplace_back(0.500008, 0, 0       );

      createChild("opacities", "DataVector1f");

      auto colorCP = createChild("colorControlPoints", "DataVector4f").nodeAs<DataVector4f>();
      colorCP->v.emplace_back(0.0, 0       , 0, 0.562493);
      colorCP->v.emplace_back(1.0f/6.0f, 0       , 0, 1       );
      colorCP->v.emplace_back(2.0f/6.0f, 0       , 1, 1       );
      colorCP->v.emplace_back(3.0f/6.0f, 0.500008, 1, 0.500008);
      colorCP->v.emplace_back(4.0f/6.0f, 1       , 1, 0       );
      colorCP->v.emplace_back(5.0f/6.0f, 1       , 0, 0       );
      colorCP->v.emplace_back(1.0, 0.500008, 0, 0       );
      auto opacityCP = createChild("opacityControlPoints", "DataVector2f").nodeAs<DataVector2f>();
      opacityCP->v.emplace_back(0.f, 0.f);
      opacityCP->v.emplace_back(1.f, 1.f);
    }

    float TransferFunction::interpolateOpacity(const DataBuffer &controlPoints,
                                              float x)
    {
      auto first = controlPoints.get<vec2f>(0);
      if (x <= first.x)
        return first.y;

      for (uint32_t i = 1; i < controlPoints.size(); i++) {
        auto current  = controlPoints.get<vec2f>(i);
        auto previous = controlPoints.get<vec2f>(i - 1);
        if (x <= current.x) {
          const float t = (x - previous.x) / (current.x - previous.x);
          return (1.0 - t) * previous.y + t * current.y;
        }
      }

      auto last = controlPoints.get<vec2f>(controlPoints.size() - 1);
      return last.y;
    }

    vec3f TransferFunction::interpolateColor(const DataBuffer &controlPoints,
                                              float x)
    {
      auto first = controlPoints.get<vec4f>(0);
      if (x <= first.x)
        return vec3f(first.y,first.z,first.w);

      for (uint32_t i = 1; i < controlPoints.size(); i++) {
        auto current  = controlPoints.get<vec4f>(i);
        auto previous = controlPoints.get<vec4f>(i - 1);
        if (x <= current.x) {
          const float t = (x - previous.x) / (current.x - previous.x);
          return (1.0 - t) * vec3f(previous.y,previous.z,previous.w)+ t * vec3f(current.y,current.z,current.w);
        }
      }

      auto last = controlPoints.get<vec4f>(controlPoints.size() - 1);
      return vec3f(last.x,last.y,last.z);
    }

    void TransferFunction::computeOpacities()
    {
      auto numSamples = child("numSamples").valueAs<int>();

      float x0 = 0.f;
      float dx = (1.f - x0) / (numSamples-1);

      auto controlPoints = child("opacityControlPoints").nodeAs<DataBuffer>();
      auto opacities     = child("opacities").nodeAs<DataVector1f>();

      opacities->v.clear();
      for (int i = 0; i < numSamples; i++)
        opacities->push_back(interpolateOpacity(*controlPoints, i * dx));
    }

    void TransferFunction::computeColors()
    {
      auto numSamples = child("numSamples").valueAs<int>();

      float x0 = 0.f;
      float dx = (numSamples == 1 ) ? 0.0f : (1.f - x0) / (numSamples-1);

      auto controlPoints = child("colorControlPoints").nodeAs<DataBuffer>();
      auto colors        = child("colors").nodeAs<DataVector3f>();
      colors->v.clear();

      for (int i = 0; i < numSamples; i++)
        colors->push_back(interpolateColor(*controlPoints, i * dx));
    }

    void TransferFunction::updateChildDataValues()
    {
      computeOpacities();
      computeColors();
    }

    void TransferFunction::preCommit(RenderContext &)
    {
      auto ospTransferFunction = valueAs<OSPTransferFunction>();
      if (!ospTransferFunction) {
        ospTransferFunction = ospNewTransferFunction("piecewise_linear");
        setValue(ospTransferFunction);
      }

      computeOpacities();
      computeColors();
    }

    void TransferFunction::postCommit(RenderContext &)
    {
      ospCommit(valueAs<OSPTransferFunction>());
    }

    std::string TransferFunction::toString() const
    {
      return "ospray::sg::TransferFunction";
    }

    void TransferFunction::loadParaViewTF(std::string fileName)
    {
      std::ifstream paraViewFile(fileName);
      if (!paraViewFile.is_open()) {
        throw std::runtime_error("#ospCvtParaViewTfcn: Error - failed to open file " + fileName);
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
      setName(tfcnName);

      if (paraViewFcn["ColorSpace"].type() == Json::stringValue
                      && paraViewFcn["ColorSpace"].asString() == "Diverging") {
        std::cout << "#ospCvtParaViewTfcn: WARNING: ParaView's diverging color space "
                  << "interpolation is not supported, colors may be incorrect\n";
      }


      auto& colorCP = *child("colorControlPoints").nodeAs<DataVector4f>();
      auto& opacityCP = *child("opacityControlPoints").nodeAs<DataVector2f>();
      colorCP.clear();
      opacityCP.clear();
      // Read the value, opacity pairs and ignore the strange extra 0.5, 0 entries
      std::cout << "#ospCvtParaViewTfcn: Reading value, opacity pairs\n";
      Json::Value pvOpacities = paraViewFcn["Points"];
      if (!pvOpacities.isArray()) {
        std::cout << "#ospCvtParaViewTfcn: No opacity data, setting default of linearly increasing [0, 1]\n";
        opacityCP.push_back(vec2f(0.f, 0.f));
        opacityCP.push_back(vec2f(1.f, 1.f));
      } else {
        // We the first 2 of every 4 values which are the (value, opacity) pair
        // followed by some random (0.5, 0) value pair that ParaView throws in there
        for (Json::Value::ArrayIndex i = 0; i < pvOpacities.size(); i += 4) {
          const float val = pvOpacities[i].asFloat();
          const float opacity = pvOpacities[i + 1].asFloat();
          opacityCP.push_back(vec2f(val, opacity));
        }
      }
      const float dataValueMin = opacityCP[0].x;
      const float dataValueMax = opacityCP.v.back().x;
      // Re-scale the opacity value entries into [0, 1]
      for (auto &v : opacityCP.v) {
        v.x = (v.x - dataValueMin) / (dataValueMax - dataValueMin);
      }

      // Read the (val, RGB) pairs
      std::cout << "#ospCvtParaViewTfcn: Reading value, r, g, b tuples\n";
      Json::Value pvColors = paraViewFcn["RGBPoints"];
      if (!pvColors.isArray()) {
        throw std::runtime_error("#ospCvtParaViewTfcn: Error - failed to find value, r, g, b 'RGBPoints' array\n");
      }
      for (Json::Value::ArrayIndex i = 0; i < pvColors.size(); i += 4) {
        const float val = (pvColors[i].asFloat() - dataValueMin) / (dataValueMax - dataValueMin);
        const float r = pvColors[i + 1].asFloat();
        const float g = pvColors[i + 2].asFloat();
        const float b = pvColors[i + 3].asFloat();
        colorCP.push_back(vec4f(val, r, g, b));
      }
    }

    OSP_REGISTER_SG_NODE(TransferFunction);

  } // ::ospray::sg
} // ::ospray
