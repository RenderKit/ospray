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

#include "render/Renderer.h"
#include "common/Material.h"

namespace ospray {

  struct PathTracer : public Renderer
  {
    PathTracer();
    virtual ~PathTracer() override;
    virtual std::string toString() const override;
    virtual void commit() override;

    void generateGeometryLights(const Model *const, const affine3f& xfm,
                                float *const areaPDF);
    void destroyGeometryLights();

    std::vector<void*> lightArray; // the 'IE's of the XXXLights
    size_t geometryLights {0}; // number of GeometryLights at beginning of lightArray
    std::vector<float> areaPDF; // pdfs wrt. area of regular (not instanced) geometry lights
    Data *lightData;
  };

}// ::ospray

