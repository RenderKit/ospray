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

// ospray
#include "render/Renderer.h"

namespace ospray {
  namespace mpi {

    struct DistributedRaycastRenderer : public Renderer
    {
      DistributedRaycastRenderer();
      virtual ~DistributedRaycastRenderer() = default;//TODO!

      void commit() override;

      float renderFrame(FrameBuffer *fb, const uint32 fbChannelFlags) override;

      std::string toString() const override;
    };

  } // ::ospray::mpi
} // ::ospray
