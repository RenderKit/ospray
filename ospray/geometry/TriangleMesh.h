// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

  struct OSPRAY_SDK_INTERFACE TriangleMesh : public Geometry
  {
    TriangleMesh();
    virtual ~TriangleMesh() override = default;

    virtual std::string toString() const override;

    virtual void commit() override;

    virtual size_t numPrimitives() const override;

   protected:
    bool huge_mesh{false};

    int *index;       //!< mesh's triangle index array
    float *vertex;    //!< mesh's vertex array
    float *normal;    //!< mesh's vertex normal array
    vec4f *color;     //!< mesh's vertex color array
    vec2f *texcoord;  //!< mesh's vertex texcoord array

    Ref<Data> indexData;    /*!< triangle indices (A,B,C,materialID) */
    Ref<Data> vertexData;   /*!< vertex position (vec3fa) */
    Ref<Data> normalData;   /*!< vertex normal array (vec3fa) */
    Ref<Data> colorData;    /*!< vertex color array (vec3fa) */
    Ref<Data> texcoordData; /*!< vertex texcoord array (vec2f) */

    size_t numTris{0};
    size_t numVerts{0};

    size_t numCompsInTri{0};
    size_t numCompsInVtx{0};
    size_t numCompsInNor{0};

   private:
    void createEmbreeGeometry() override;
  };

}  // namespace ospray
