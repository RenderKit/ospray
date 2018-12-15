// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "common/Data.h"

namespace ospray {

  /*! \defgroup geometry_quadmesh  Quad  Meshes ("quadmesh")

    \brief Implements a traditional quad  mesh (indexed face set) geometry

    \ingroup ospray_supported_geometries

    A quad mesh is created via calling \ref ospNewGeometry with type
    string "quads".

    Once created, a quadmesh recognizes the following parameters
    <pre>
    Data<vec3f> or Data<vec3fa> "position"        // vertex array
    Data<vec3i> or Data<vec3ia> "index"           // index array (four int32 values per quad)
                                                  // each int32 value is index into the vertex arrays
    Data<vec3f> or Data<vec3fa> "normal"          // vertex normals
    Data<vec4f>                 "color"           // vertex colors
    Data<vec2f>                 "texcoord"        // texture coordinates
    uint32                      "geom.materialID" // material ID for the whole mesh
    Data<uint32>                "prim.materialID" // per quad  materials, indexing into "materialList"
    </pre>

    The functionality for this geometry is implemented via the
    \ref ospray::QuadMesh class.
  */



  /*! \brief A Quad Mesh / Indexed Face Set Geometry

    A Quad Mesh is a geometry registered under the name
    "quadmesh", and has several parameters to specify its contained
    quads; they are specified via data arrays of the proper form:
      -  a 'position' array (Data<vec3f> or Data<vec3fa> type)
      -  a 'normal' array (Data<vec3f> or Data<vec3fa> type)
      -  a 'color' array (Data<vec4f> type)
      -  a 'texcoord' array (Data<vec2f> type)
      -  an 'index' array (Data<vec3i> or Data<vec3ia> type)

    Indices specified in the 'index' array refer into the vertex arrays;
    each value is the index of a _vertex_ (it, not a byte-offset, nor an
    index into a float array, but an index into an array of vertices).

    Materials can be set either via 'geom.materialID' (uint32 type) for
    the whole mesh, or per quad via
      - a 'prim.materialID' array (Data<uint32> type), indexing into
      - a 'materialList' array (holding the OSPMaterial pointers)
   */
  struct OSPRAY_SDK_INTERFACE QuadMesh : public Geometry
  {
    QuadMesh();
    virtual ~QuadMesh() override = default;
    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    int    *index;  //!< mesh's quad index array
    float  *vertex; //!< mesh's vertex array
    float  *normal; //!< mesh's vertex normal array
    vec4f  *color;  //!< mesh's vertex color array
    vec2f  *texcoord; //!< mesh's vertex texcoord array
    uint32 *prim_materialID; //!< per-primitive material ID
    int geom_materialID;

    Ref<Data> indexData;  /*!< quad indices (A,B,C,materialID) */
    Ref<Data> vertexData; /*!< vertex position (vec3fa) */
    Ref<Data> normalData; /*!< vertex normal array (vec3fa) */
    Ref<Data> colorData;  /*!< vertex color array (vec3fa) */
    Ref<Data> texcoordData; /*!< vertex texcoord array (vec2f) */
    Ref<Data> prim_materialIDData;  /*!< data array for per-prim material ID (uint32) */

    #define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID
    uint32 eMeshID{RTC_INVALID_ID};   /*!< embree quad  mesh handle */
  };

} // ::ospray
