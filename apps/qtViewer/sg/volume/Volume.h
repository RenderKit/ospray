// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "sg/common/TransferFunction.h"

namespace ospray {
  namespace sg {

    /*! a geometry node - the generic geometry node */
    struct Volume : public sg::Node {
      Volume() {};

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds() = 0;

      SG_NODE_DECLARE_MEMBER(Ref<TransferFunction>,transferFunction,TransferFunction);    
    };
    
    struct ProceduralTestVolume : public Volume {
      //! \brief constructor
      ProceduralTestVolume(); 

      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const;

      //! return bounding box of all primitives
      virtual box3f getBounds();

      //! \brief Initialize this node's value from given XML node 
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      /*! \brief 'render' the object to ospray */
      virtual void render(RenderContext &ctx);

      SG_NODE_DECLARE_MEMBER(vec3i,dimensions,Dimensions);    

      //! coefficients we use for generating the procedural 'noise'
      /*! we generate a procedural volume using the function cos^2(p0
          + p1*x + p2*y + p3 * z + p4*xy + p5 * yz + p6 * xz + p7 *
          xyz), where x,y,z in [0,1] are the local coordinates inside
          the volume, and p_i are the 8 values in the coeff[] array */
      float coeff[1+3+3+1];
      OSPVolume volume;
    };
    
  } // ::ospray::sg
} // ::ospray


