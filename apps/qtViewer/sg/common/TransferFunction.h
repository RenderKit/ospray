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
      virtual void setFromXML(const xml::Node *const node) {}
      virtual void commit();
      
      /*! set a new color map array */
      void setColorMap(const std::vector<vec3f> &colorArray);
      /*! set a new alpha map array */
      void setAlphaMap(const std::vector<float> &alphaArray);

      /*! return the ospray handle for this xfer fct, so we can assign
          it to ospray obejcts that need a reference to the ospray
          version of this xf */
      OSPTransferFunction getOSPHandle() const { return ospTransferFunction; };
    protected:
      OSPTransferFunction ospTransferFunction;
      OSPData ospColorData;
      OSPData ospAlphaData;

      std::vector<vec3f> colorArray;
      std::vector<float> alphaArray;
    };    
    
  } // ::ospray::sg
} // ::ospray


