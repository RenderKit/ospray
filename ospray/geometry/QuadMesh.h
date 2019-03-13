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

  struct OSPRAY_SDK_INTERFACE QuadMesh : public Geometry
  {
    QuadMesh();
    virtual ~QuadMesh() override = default;
    virtual std::string toString() const override;
    virtual void finalize(World *model) override;

    int *index;               //!< mesh's quad index array
    float *vertex;            //!< mesh's vertex array
    float *normal;            //!< mesh's vertex normal array
    vec4f *color;             //!< mesh's vertex color array
    vec2f *texcoord;          //!< mesh's vertex texcoord array
    uint32 *prim_materialID;  //!< per-primitive material ID
    int geom_materialID;

    Ref<Data> indexData;           /*!< quad indices (A,B,C,materialID) */
    Ref<Data> vertexData;          /*!< vertex position (vec3fa) */
    Ref<Data> normalData;          /*!< vertex normal array (vec3fa) */
    Ref<Data> colorData;           /*!< vertex color array (vec3fa) */
    Ref<Data> texcoordData;        /*!< vertex texcoord array (vec2f) */
    Ref<Data> prim_materialIDData; /*!< data array for per-prim material ID
                                      (uint32) */

#define RTC_INVALID_ID RTC_INVALID_GEOMETRY_ID
    uint32 eMeshID{RTC_INVALID_ID}; /*!< embree quad  mesh handle */
  };

}  // namespace ospray
