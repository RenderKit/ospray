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

#pragma once

#include "sg/common/Node.h"
#include "sg/geometry/Geometry.h"
#include "sg/common/Data.h"

namespace ospray {
  namespace sg {

    /*! A Simple Triangle Mesh that stores vertex, normal, texcoord,
        and vertex color in separate arrays */
    struct TriangleMesh : public sg::Geometry {

      //! constructor
      TriangleMesh() : Geometry("trianglemesh"), ospGeometry(NULL) {};
      
      /*! \brief returns a std::string with the c++ name of this class */
      virtual    std::string toString() const { return "ospray::sg::Geometry"; }

      //! return bounding box of all primitives
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
      virtual void setFromXML(const xml::Node *const node, const unsigned char *binBasePtr);

      OSPGeometry         ospGeometry;
      
      // to allow memory-mapping triangle arrays (or in general,
      // sharing data with an application) we use data arrays, not std::vector's

      //! vertex (position) array
      std::shared_ptr<DataBuffer> vertex;
      
      //! vertex normal array. empty means 'not present'
      std::shared_ptr<DataBuffer> normal;
      
      //! vertex color array. empty means 'not present'
      std::shared_ptr<DataBuffer> color;

      //! vertex texture coordinate array. empty means 'not present'
      std::shared_ptr<DataBuffer> texcoord;

      //! triangle indices
      std::shared_ptr<DataBuffer> index;
    };


    /*! A special triangle mesh that allows per-triangle materials */
    struct PTMTriangleMesh : public sg::Geometry {

      /*! triangle with per-triangle material ID */
      struct Triangle {
        uint32_t vtxID[3], materialID;
      };

      //! constructor
      PTMTriangleMesh() : Geometry("trianglemesh"), ospGeometry(NULL) {};
      
      // return bounding box of all primitives
      virtual box3f getBounds();

      /*! 'render' the nodes */
      virtual void render(RenderContext &ctx);

      OSPGeometry         ospGeometry;

      /*! \brief "material list" for this trianglemesh 
        
        If non-empty, the 'Triangle::materialID' indexes into this
        list; if empty, all trianlges should use the
        Geometry::material no matter what Triangle::materialID is set
       */
      std::vector<std::shared_ptr<sg::Material> > materialList;
      std::vector<uint32_t> materialIDs;

      // to allow memory-mapping triangle arrays (or in general,
      // sharing data with an application) we use data arrays, not std::vector's

      //! vertex (position) array
      std::shared_ptr<DataBuffer> vertex;
      
      //! vertex normal array. empty means 'not present'
      std::shared_ptr<DataBuffer> normal;
      
      //! vertex color array. empty means 'not present'
      std::shared_ptr<DataBuffer> color;

      //! vertex texture coordinate array. empty means 'not present'
      std::shared_ptr<DataBuffer> texcoord;

      //! triangle indices
      std::shared_ptr<DataBuffer> index;
      
      //! material IDs
      OSPData primMatIDs;
   };

  } // ::ospray::sg
} // ::ospray


