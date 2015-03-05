/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "sg/common/Node.h"

namespace ospray {
  namespace sg {

    /*! \brief a *tabulated* transfer function realized through
        uniformly spaced color and alpha values between which the
        value will be linearly interpolated (similar to a 1D texture
        for each) */
    struct TransferFunction : public sg::Node {

      TransferFunction() : ospTransferFunction(NULL), ospColorData(NULL), ospAlphaData(NULL) { setDefaultValues(); }

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


