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

#include "ospray/render/Renderer.h"
#include "ospray/common/Material.h"

namespace ospray {
  struct PathTracer : public Renderer {
    PathTracer();
    virtual std::string toString() const { return "ospray::PathTracer"; }
    virtual void commit();
    virtual Material *createMaterial(const char *type);
    virtual float renderFrame(FrameBuffer *fb, const uint32 channelFlags) override;

    std::vector<void*> lightArray; // the 'IE's of the XXXLights
    Data *lightData;
  };
}

