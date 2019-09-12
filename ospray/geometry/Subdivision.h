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

  struct OSPRAY_SDK_INTERFACE Subdivision : public Geometry
  {
    Subdivision() = default;
    virtual ~Subdivision() override = default;

    virtual std::string toString() const override;

    virtual void commit() override;

    virtual size_t numPrimitives() const override;

    LiveGeometry createEmbreeGeometry() override;

   protected:
    float level{0.f};

    Ref<const DataT<vec3f>> vertexData;
    Ref<const DataT<vec4f>> colorsData;
    Ref<const DataT<vec2f>> texcoordData;
    Ref<const DataT<uint32_t>> indexData;
    Ref<const DataT<float>> indexLevelData;
    Ref<const DataT<uint32_t>> facesData;
    Ref<const DataT<vec2ui>> edge_crease_indicesData;
    Ref<const DataT<float>> edge_crease_weightsData;
    Ref<const DataT<uint32_t>> vertex_crease_indicesData;
    Ref<const DataT<float>> vertex_crease_weightsData;
  };

}  // namespace ospray
