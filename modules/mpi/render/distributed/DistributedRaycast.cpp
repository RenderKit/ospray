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

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
// ospray
#include "DistributedRaycast.h"
#include "../MPILoadBalancer.h"
#include "../../fb/DistributedFrameBuffer.h"
// ispc exports
#include "DistributedRaycast_ispc.h"

namespace ospray {
  namespace mpi {

    // DistributedRaycastRenderer definitions /////////////////////////////////

    DistributedRaycastRenderer::DistributedRaycastRenderer()
    {
      ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);
    }

    void DistributedRaycastRenderer::commit()
    {
      Renderer::commit();
      if (hasParam("clipBox.lower") && hasParam("clipBox.upper")) {
        box3f clipBox;
        clipBox.lower = getParam3f("clipBox.lower", vec3f(inf));
        clipBox.upper = getParam3f("clipBox.upper", vec3f(neg_inf));
        ispc::DistributedRaycastRenderer_setClipBox(ispcEquivalent, (ispc::box3f*)&clipBox);
      }
    }

    float DistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
                                                  const uint32 channelFlags)
    {
      auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
      dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);

      return TiledLoadBalancer::instance->renderFrame(this, fb, channelFlags);
    }

    std::string DistributedRaycastRenderer::toString() const
    {
      return "ospray::mpi::DistributedRaycastRenderer";
    }

    OSP_REGISTER_RENDERER(DistributedRaycastRenderer, mpi_raycast);

  } // ::ospray::mpi
} // ::ospray

