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

#undef NDEBUG

#include "TransferFunction.h"

namespace ospray {
  namespace sg {

    //! constructor
    TransferFunction::TransferFunction()
    {
      setDefaultValues();
      createChildNode("valueRange", "vec2f", vec2f(0.f,1.f));
      createChildNode("numSamples", "int", 256);
    }

    // //! \brief Sets a new 'texture map' to be used for the color mapping
    void TransferFunction::setColorMap(const std::vector<vec3f> &colorArray)
    {
      if (ospColorData) {
        ospRelease(ospColorData);
        ospColorData = nullptr;
      }

      this->colorArray.clear();
      for (uint32_t i = 0; i < colorArray.size(); ++i)
        this->colorArray.push_back({i, colorArray[i]});

      markAsModified();
    }

    //! \brief Sets a new 'texture map' to be used for the alpha mapping
    void TransferFunction::setAlphaMap(const std::vector<vec2f> &alphaArray)
    {
      if (ospAlphaData) {
        ospRelease(ospAlphaData);
        ospAlphaData = nullptr;
      }
      this->alphaArray.clear();
      for (const auto &alpha : alphaArray)
        this->alphaArray.push_back({alpha.x, alpha.y});

      markAsModified();
    }

    const std::vector<std::pair<float, float>> &TransferFunction::alphas() const
    {
      return alphaArray;
    }

    float TransferFunction::interpolatedAlpha(float x)
    {
      if (x <= alphaArray.front().first)
        return alphaArray.front().second;

      for (uint32_t i = 1; i < alphaArray.size(); i++) {
        if (x <= alphaArray[i].first) {
          const float t = (x - alphaArray[i - 1].first)
            / (alphaArray[i].first - alphaArray[i-1].first);
          return (1.0 - t) * alphaArray[i-1].second + t * alphaArray[i].second;
        }
      }
      return alphaArray.back().second;
    }

    OSPTransferFunction TransferFunction::handle() const
    {
      return ospTransferFunction;
    }

    void TransferFunction::preCommit(RenderContext &ctx)
    {
      if (!ospTransferFunction) {
        ospTransferFunction = ospNewTransferFunction("piecewise_linear");
        setValue((OSPObject)ospTransferFunction);
      }

      vec2f valueRange = child("valueRange").valueAs<vec2f>();
      ospSetVec2f(ospTransferFunction,"valueRange",{valueRange.x,valueRange.y});

      if (ospColorData == nullptr && colorArray.size()) {
        // for now, no resampling - just use the colors ...
        vec3f *colors = (vec3f*)alloca(sizeof(vec3f)*colorArray.size());
        for (uint32_t i = 0; i < colorArray.size(); i++)
          colors[i] = colorArray[i].second;
        ospColorData = ospNewData(colorArray.size(),OSP_FLOAT3,colors);
        ospCommit(ospColorData);
        ospSetData(ospTransferFunction,"colors",ospColorData);
      }

      if (ospAlphaData == nullptr && alphaArray.size()) {
        float *alpha = (float*)alloca(sizeof(float)*numSamples);
        float x0 = alphaArray.front().first;
        float dx = (alphaArray.back().first - x0) / (numSamples-1);

        for (int i=0;i<numSamples;i++)
          alpha[i] = interpolatedAlpha(i * dx);

        ospAlphaData = ospNewData(numSamples,OSP_FLOAT,alpha);
        ospCommit(ospAlphaData);
        ospSetData(ospTransferFunction,"opacities",ospAlphaData);
      }
    }

    void TransferFunction::postCommit(RenderContext &ctx)
    {
      ospCommit(ospTransferFunction);
    }


    void TransferFunction::setFromXML(const xml::Node& node,
                                      const unsigned char *binBasePtr)
    {
      setDefaultValues();

      xml::for_each_child_of(node,[&](const xml::Node &child) {
          // -------------------------------------------------------
          // colors
          // -------------------------------------------------------
          if (child.name == "colors" || child.name == "color") {
            colorArray.clear();
            char *cont = strdup(child.content.c_str());
            assert(cont);

            const char *c = strtok(cont,",\n");
            while (c) {
              colorArray.push_back(
                std::pair<float,vec3f>(colorArray.size(),toVec3f(c))
              );
              c = strtok(nullptr,",\n");
            }

            free(cont);
          }

          // -------------------------------------------------------
          // alpha
          // -------------------------------------------------------
          if (child.name == "alphas" || child.name == "alpha") {
            alphaArray.clear();
            char *cont = strdup(child.content.c_str());

            assert(cont);
            const char *c = strtok(cont,",\n");
            while (c) {
              vec2f cp = toVec2f(c);
              alphaArray.push_back(std::pair<float,float>(cp.x,cp.y));
              c = strtok(nullptr,",\n");
            }

            free(cont);
          }
        });
    }

    //! \brief Initialize this node's value from given corresponding XML node
    void TransferFunction::setDefaultValues()
    {
      static float col[7][3] = {{0         , 0           , 0.562493 },
                                {0         , 0           , 1        },
                                {0         , 1           , 1        },
                                {0.500008  , 1           , 0.500008 },
                                {1         , 1           , 0        },
                                {1         , 0           , 0        },
                                {0.500008  , 0           , 0        }};
      
      colorArray.clear();
      for (int i = 0; i < 7; i++) {
        colorArray.push_back(
          std::pair<float, vec3f>(i, vec3f(col[i][0], col[i][1], col[i][2]))
        );
      }

      alphaArray.clear();
      alphaArray.push_back(std::pair<float,float>(0.f,0.f));
      alphaArray.push_back(std::pair<float,float>(1.f,1.f));
    }

    std::string TransferFunction::toString() const
    {
      return "ospray::sg::TransferFunction";
    }

    OSP_REGISTER_SG_NODE(TransferFunction);

  } // ::ospray::sg
} // ::ospray
