// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

      OSPGeometry         ospGeometry;
      
#if 1
      // to allow memory-mapping triangle arrays (or in general,
      // sharing data with an application) we use data arrays, not std::vector's

      //! vertex (position) array
      Ref<DataBuffer> vertex;
      
      //! vertex normal array. empty means 'not present'
      Ref<DataBuffer> normal;
      
      //! vertex color array. empty means 'not present'
      Ref<DataBuffer> color;

      //! vertex texture coordinate array. empty means 'not present'
      Ref<DataBuffer> texcoord;

      //! triangle indices
      Ref<DataBuffer> index;
#else
      //! vertex (position) array
      std::vector<vec3fa> vertex;

      //! vertex normal array. empty means 'not present'
      std::vector<vec3fa> normal;

      //! vertex color array. empty means 'not present'
      std::vector<vec3fa> color;

      //! vertex texture coordinate array. empty means 'not present'
      std::vector<vec2f> texcoord;

      //! triangle indices
      std::vector<Triangle> index;
#endif
    };


    /*! A special triangle mesh that allows per-triangle materials */
    struct PTMTriangleMesh : public sg::Geometry {

      /*! triangle with per-triangle material ID */
      struct Triangle {
        uint32 vtxID[3], materialID;
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
      std::vector<Ref<sg::Material> > materialList;
      std::vector<uint32> materialIDs;

#if 1
      // to allow memory-mapping triangle arrays (or in general,
      // sharing data with an application) we use data arrays, not std::vector's

      //! vertex (position) array
      Ref<DataBuffer> vertex;
      
      //! vertex normal array. empty means 'not present'
      Ref<DataBuffer> normal;
      
      //! vertex color array. empty means 'not present'
      Ref<DataBuffer> color;

      //! vertex texture coordinate array. empty means 'not present'
      Ref<DataBuffer> texcoord;

      //! triangle indices
      Ref<DataBuffer> index;
      
      //! material IDs
      OSPData primMatIDs;
#else
      //! vertex (position) array
      std::vector<vec3fa> vertex;

      //! vertex normal array
      std::vector<vec3fa> normal;

      //! vertex texture coordinate array
      std::vector<vec2f> texcoord;
      //! triangle indices
#endif 
   };

  } // ::ospray::sg
} // ::ospray


