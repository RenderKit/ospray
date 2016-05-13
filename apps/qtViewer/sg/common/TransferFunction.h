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

#pragma once

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    /*! \brief a *tabulated* transfer function realized through
        uniformly spaced color and alpha values between which the
        value will be linearly interpolated (similar to a 1D texture
        for each) */
    struct TransferFunction : public sg::Node 
    {
      //! constructor
      TransferFunction();

      //! \brief initialize color and alpha arrays to 'some' useful values
      void setDefaultValues();

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::TransferFunction"; }

      //! \brief creates ospray-side object(s) for this node
      virtual void render(RenderContext &ctx);

      //! \brief Initialize this node's value from given corresponding XML node 
      virtual void setFromXML(const xml::Node *const node, 
                              const unsigned char *binBasePtr);
      virtual void commit();
      
      void setValueRange(const vec2f &range);

      // /*! set a new color map array (using array of uniformly samples colors) */
      void setColorMap(const std::vector<vec3f> &colorArray);
      /*! set a new alpha map array - x coordinate is point pos, y is point alpha value */
      void setAlphaMap(const std::vector<vec2f> &alphaArray);

      const std::vector<std::pair<float,float> > &getAlphaArray() const
      { return alphaArray; }

      float getInterpolatedAlphaValue(float x);

      /*! return the ospray handle for this xfer fct, so we can assign
        it to ospray obejcts that need a reference to the ospray
        version of this xf */
      OSPTransferFunction getOSPHandle() const { return ospTransferFunction; };
    protected:
      OSPTransferFunction ospTransferFunction;
      OSPData ospColorData;
      OSPData ospAlphaData;
      vec2f valueRange;
      // number of samples we'll use in the colordata and alphadata arrays
      int numSamples;

      // array of (x,color(x)) color samples; the first and last x
      // determine the range of x'es, all values will be resampled
      // uniformly into this range. samples must be sorted by x
      // coordinate, and must span a non-empty range of x coordinates
      std::vector<std::pair<float,vec3f> > colorArray;
      // array of (x,alpha(x)) opacity samples; otherwise same as colorArray
      std::vector<std::pair<float,float> > alphaArray;
    };    
    
  } // ::ospray::sg
} // ::ospray


