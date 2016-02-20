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
#include "sg/common/TransferFunction.h"
#include "sg/geometry/Geometry.h"

namespace ospray {
  namespace sg {

    /*! simple spheres, with all of the key info - position, radius,
        and a int32_t type specifier baked into each sphere  */
    struct Spheres : public sg::Geometry {
      struct Sphere { 
        vec3f position;
        float radius;
        uint32_t typeID;
        
        // constructor
        Sphere(vec3f position, float radius, uint typeID=0);
        
        // return the bounding box
        inline box3f getBounds() const
        { return box3f(position-vec3f(radius),position+vec3f(radius)); };
      };

      //! constructor
      Spheres() : Geometry("spheres"), ospGeometry(NULL) {};
      
      // return bounding box of all primitives
      virtual box3f getBounds();

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx);

      //! \brief Initialize this node's value from given XML node 
      /*!
        \detailed This allows a plug-and-play concept where a XML
        file can specify all kind of nodes wihout needing to know
        their actual types: The XML parser only needs to be able to
        create a proper C++ instance of the given node type (the
        OSP_REGISTER_SG_NODE() macro will allow it to do so), and can
        tell the node to parse itself from the given XML content and
        XML children 
        
        \param node The XML node specifying this node's fields

        \param binBasePtr A pointer to an accompanying binary file (if
        existant) that contains additional binary data that the xml
        node fields may point into
      */
      void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      OSPGeometry         ospGeometry;
      std::vector<Sphere> sphere;
    };

  } // ::ospray::sg
} // ::ospray


