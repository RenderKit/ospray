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

#include "Geometry.h"
#include "ospray/common/Data.h"

namespace ospray {

  /*! \defgroup geometry_trianglemesh Triangle Meshes ("trianglemesh") 

    \brief Implements a traditional triangle mesh (indexed face set) geometry

    \ingroup ospray_supported_geometries

    A triangle mesh is created via \ref ospNewTriangleMesh, or via
    calling \ref ospNewGeometry with type string \"trianglemesh\".

    Once created, a trianglemesh recognizes the following parameters
    <pre>
    data<vec3fa> "position"  // vertex array
    data<vec3i>  "index"     // index array (three int32 values per triangles)
                             // each int32 value is index into 'position' array
    </pre>

    The functionality for this geometry is implemented via the
    \ref ospray::TriangleMesh class.
  */



  /*! \brief A Triangle Mesh / Indexed Face Set Geometry

    A Triangle Mesh is a geometry registered under the name
    "trianglemesh", and has two parameters to specify its contained
    triangles; both specified via data arrays of the proper form: a
    "position" array (Data<vec3f> type), and a "index" array
    (Data<vec3i> or Data<vec3ia> type); indices specified in the
    'index' array refer into the vertex array; each value is the index
    of a _vertex_ (it, not a byte-offset, nor an index into a float
    array, but an index into an array of vertices).
   */
  struct TriangleMesh : public Geometry
  {

    TriangleMesh();
    virtual std::string toString() const { return "ospray::TriangleMesh"; }
    virtual void finalize(Model *model);

    const vec3i  *index;  //!< mesh's triangle index array
    const vec3fa *vertex; //!< mesh's vertex array
    const vec3fa *normal; //!< mesh's vertex normal array
    const vec4f  *color;  //!< mesh's vertex color array
    const vec2f  *texcoord; //!< mesh's vertex texcoord array
    const uint32 *prim_materialID; //!< per-primitive material ID
    Material **materialList; //!< per-primitive material list
    int geom_materialID;

    Ref<Data> indexData;  /*!< triangle indices (A,B,C,materialID) */
    Ref<Data> vertexData; /*!< vertex position (vec3fa) */
    Ref<Data> normalData; /*!< vertex normal array (vec3fa) */
    Ref<Data> colorData;  /*!< vertex color array (vec3fa) */
    Ref<Data> texcoordData; /*!< vertex texcoord array (vec2f) */
    Ref<Data> prim_materialIDData;  /*!< data array for per-prim material ID (uint32) */
    Ref<Data> materialListData; /*!< data array for per-prim materials */
    uint32    eMesh;   /*!< embree triangle mesh handle */

    void** ispcMaterialPtrs; /*!< pointers to ISPC equivalent materials */
  };

} // ::ospray
