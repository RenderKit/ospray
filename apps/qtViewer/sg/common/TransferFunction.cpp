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

    //! constructor
    TransferFunction::TransferFunction() 
      : ospTransferFunction(NULL), 
        ospColorData(NULL), 
        ospAlphaData(NULL)
    { 
      setDefaultValues(); 
    }

    //! \brief Sets a new 'texture map' to be used for the color mapping
    void TransferFunction::setColorMap(const std::vector<vec3f> &colorArray)
    {
      if (ospColorData) { ospRelease(ospColorData); ospColorData = NULL; }
      this->colorArray = colorArray;
    }

    //! \brief Sets a new 'texture map' to be used for the alpha mapping
    void TransferFunction::setAlphaMap(const std::vector<float> &alphaArray)
    {
      if (ospAlphaData) { ospRelease(ospAlphaData); ospAlphaData = NULL; }
      this->alphaArray = alphaArray;
    }

    //! \brief commit the current field values to ospray
    void TransferFunction::commit() 
    {
      if (ospColorData == NULL) {
        ospColorData = ospNewData(colorArray.size(),OSP_FLOAT3,&colorArray[0]); 
        ospCommit(ospColorData);
        ospSetData(ospTransferFunction,"colors",ospColorData);
        lastModified = TimeStamp::now();
      }
      if (ospAlphaData == NULL) {
        ospAlphaData = ospNewData(alphaArray.size(),OSP_FLOAT,&alphaArray[0]); 
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

    //! \brief Initialize this node's value from given corresponding XML node 
    void TransferFunction::setDefaultValues()
    {
      colorArray.clear();
      colorArray.push_back(ospcommon::vec3f(0         , 0           , 0.562493   ));
      colorArray.push_back(ospcommon::vec3f(0         , 0           , 1          ));
      colorArray.push_back(ospcommon::vec3f(0         , 1           , 1          ));
      colorArray.push_back(ospcommon::vec3f(0.500008  , 1           , 0.500008   ));
      colorArray.push_back(ospcommon::vec3f(1         , 1           , 0          ));
      colorArray.push_back(ospcommon::vec3f(1         , 0           , 0          ));
      colorArray.push_back(ospcommon::vec3f(0.500008  , 0           , 0          ));

      alphaArray.clear();
      for (int i=0;i<colorArray.size();i++)
        alphaArray.push_back(1.f); //i/float(colorArray.size()-1));
    }

    OSP_REGISTER_SG_NODE(TransferFunction)
  } // ::ospray::sg
} // ::ospray
