// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
    using std::cout;
    using std::endl;

    //! constructor
    TransferFunction::TransferFunction() 
      : ospTransferFunction(NULL), 
        ospColorData(NULL), 
        ospAlphaData(NULL),
        numSamples(128),
        valueRange(0.f,1.f)
    { 
      setDefaultValues(); 
    }

    // //! \brief Sets a new 'texture map' to be used for the color mapping
    void TransferFunction::setColorMap(const std::vector<vec3f> &colorArray)
    {
      if (ospColorData) { ospRelease(ospColorData); ospColorData = NULL; }
      this->colorArray.clear();
      for (int i=0;i<colorArray.size();i++)
        this->colorArray.push_back(std::pair<float,vec3f>(i,colorArray[i]));
    }

    //! \brief Sets a new 'texture map' to be used for the alpha mapping
    void TransferFunction::setAlphaMap(const std::vector<vec2f> &alphaArray)
    {
      if (ospAlphaData) { ospRelease(ospAlphaData); ospAlphaData = NULL; }
      this->alphaArray.clear();
      for (int i=0;i<alphaArray.size();i++)
        this->alphaArray.push_back(std::pair<float,float>(alphaArray[i].x,alphaArray[i].y));
    }

    float TransferFunction::getInterpolatedAlphaValue(float x)
    {
      if (x <= alphaArray.front().first)
        return alphaArray.front().second;
      for (int i=1;i<alphaArray.size();i++) 
        if (x <= alphaArray[i].first)
          return
            alphaArray[i-1].second +
            (alphaArray[i].second - alphaArray[i-1].second)
            * (x-alphaArray[i-1].first) 
            / (alphaArray[i].first - alphaArray[i-1].first)
            ;
      return alphaArray.back().second;
    }

    void TransferFunction::setValueRange(const vec2f &range)
    { 
      valueRange = range; 
      lastModified = TimeStamp::now();
    }

    //! \brief commit the current field values to ospray
    void TransferFunction::commit() 
    {
      ospSetVec2f(ospTransferFunction,"valueRange",valueRange);
      if (ospColorData == NULL) {
        // for now, no resampling - just use the colors ...
        vec3f *colors = (vec3f*)alloca(sizeof(vec3f)*colorArray.size());
        for (int i=0;i<colorArray.size();i++)
          colors[i] = colorArray[i].second;
        ospColorData = ospNewData(colorArray.size(),OSP_FLOAT3,colors); 
        ospCommit(ospColorData);
        ospSetData(ospTransferFunction,"colors",ospColorData);
        lastModified = TimeStamp::now();
      }
      if (ospAlphaData == NULL) {
        float *alpha = (float*)alloca(sizeof(float)*numSamples);
        float x0 = alphaArray.front().first;
        float dx = (alphaArray.back().first - x0) / (numSamples-1);

        for (int i=0;i<numSamples;i++)
          alpha[i] = getInterpolatedAlphaValue(i * dx);

        ospAlphaData = ospNewData(numSamples,OSP_FLOAT,alpha); 
        ospCommit(ospAlphaData);
        ospSetData(ospTransferFunction,"opacities",ospAlphaData);
        lastModified = TimeStamp::now();
      }
      if (lastModified > lastCommitted) {
        lastCommitted = rdtsc();
        ospCommit(ospTransferFunction);
      }
    }

    void TransferFunction::render(RenderContext &ctx)
    {
      if (!ospTransferFunction) {
        ospTransferFunction = ospNewTransferFunction("piecewise_linear");
      }
      commit();
    }

    void TransferFunction::setFromXML(const xml::Node *const node, 
                                      const unsigned char *binBasePtr) 
    {
      setDefaultValues();

      const std::string name = node->getProp("name");
      if (name != "")
        registerNamedNode(name,this);

      for (int ii = 0; ii != node->child.size(); ii++) {
        const xml::Node *child = node->child[ii];
        // -------------------------------------------------------
        // colors
        // -------------------------------------------------------
        if (child->name == "colors" || child->name == "color") {
          colorArray.clear();
          char *cont = strdup(child->content.c_str());
          assert(cont);

          const char *c = strtok(cont,",\n");
          while (c) {
            PRINT(c);
            colorArray.push_back(std::pair<float,vec3f>(colorArray.size(),toVec3f(c)));
            c = strtok(NULL,",\n");
          }

          free(cont);
        }

        // -------------------------------------------------------
        // alpha
        // -------------------------------------------------------
        if (child->name == "alphas" || child->name == "alpha") {
          alphaArray.clear();
          char *cont = strdup(child->content.c_str());

          assert(cont);
          const char *c = strtok(cont,",\n");
          while (c) {
            vec2f cp = toVec2f(c);
            alphaArray.push_back(std::pair<float,float>(cp.x,cp.y));
            c = strtok(NULL,",\n");
          }

          free(cont);
        }
      }
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
      for (int i=0;i<7;i++)
        colorArray.push_back(std::pair<float,vec3f>(i,vec3f(col[i][0],col[i][1],col[i][2])));
      
      alphaArray.clear();
      alphaArray.push_back(std::pair<float,float>(0.f,0.f));
      alphaArray.push_back(std::pair<float,float>(1.f,1.f));
      // for (int i=0;i<colorArray.size();i++)
      //   alphaArray.push_back(std::pair<float,float>(i,1.f)); //i/float(colorArray.size()-1));
    }

    OSP_REGISTER_SG_NODE(TransferFunction)
  } // ::ospray::sg
} // ::ospray
