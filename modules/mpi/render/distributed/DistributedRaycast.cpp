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

// ospray
#include "DistributedRaycast.h"
#include "../../fb/DistributedFrameBuffer.h"
// ispc exports
#include "DistributedRaycast_ispc.h"

namespace ospray {
  namespace mpi {

    DistributedRaycastRenderer::DistributedRaycastRenderer()
    {
      ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 fbChannelFlags)
    {
      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);

      Renderer::beginFrame(fb);
      dfb->startNewFrame(errorThreshold);

      //TODO: implement the actual rendering...

      dfb->waitUntilFinished();
      Renderer::endFrame(nullptr, fbChannelFlags);

      return fb->endFrame(0.f);//TODO: can report error threshold here?
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
    }

  } // ::ospray::mpi
} // ::ospray

