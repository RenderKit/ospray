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
    Subdivision();
    virtual ~Subdivision() override = default;

    virtual std::string toString() const override;

    virtual void commit() override;

    virtual size_t numPrimitives() const override;

   protected:
    float level{0.f};

    Ref<Data> vertexData;
    Ref<Data> indexData;
    Ref<Data> facesData;
    Ref<Data> edge_crease_indicesData;
    Ref<Data> edge_crease_weightsData;
    Ref<Data> vertex_crease_indicesData;
    Ref<Data> vertex_crease_weightsData;
    Ref<Data> colorsData;
    Ref<Data> texcoordData;
    Ref<Data> indexLevelData;

    size_t numFaces{0};
    uint32_t *faces{nullptr};

    std::vector<uint32_t> generatedFacesData;

   private:
    void createEmbreeGeometry() override;
  };

}  // namespace ospray
