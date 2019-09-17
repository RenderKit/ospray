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

#include "DistributedRenderer.h"
#include "DistributedRenderer_ispc.h"

namespace ospray {
  namespace mpi {

    void DistributedRenderer::computeRegionVisibility(
        DistributedFrameBuffer *fb,
        Camera *camera,
        DistributedWorld *world,
        bool *regionVisible,
        void *perFrameData,
        Tile &tile,
        size_t jobID) const
    {
      // TODO this needs an exported function
      ispc::DistributedRenderer_computeRegionVisibility(getIE(),
                                                        fb->getIE(),
                                                        camera->getIE(),
                                                        world->getIE(),
                                                        regionVisible,
                                                        perFrameData,
                                                        (ispc::Tile &)tile,
                                                        jobID);
    }

    void DistributedRenderer::renderRegionToTile(DistributedFrameBuffer *fb,
                                                 Camera *camera,
                                                 DistributedWorld *world,
                                                 const Region &region,
                                                 void *perFrameData,
                                                 Tile &tile,
                                                 size_t jobID) const
    {
      // TODO: exported fcn
      ispc::DistributedRenderer_renderRegionToTile(getIE(),
                                                   fb->getIE(),
                                                   camera->getIE(),
                                                   world->getIE(),
                                                   &region,
                                                   perFrameData,
                                                   (ispc::Tile &)tile,
                                                   jobID);
    }

  }  // namespace mpi
}  // namespace ospray
