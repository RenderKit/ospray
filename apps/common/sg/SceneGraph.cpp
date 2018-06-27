// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "SceneGraph.h"

namespace ospray {
  namespace sg {

    Root::Root()
    {
      createChild("frameBuffer", "FrameBuffer");
      createChild("camera", "PerspectiveCamera");
    }

    std::string Root::toString() const
    {
      return "ospray::sg::Root";
    }

    void Root::preCommit(RenderContext &ctx)
    {
      if (child("camera").hasChild("aspect") &&
          child("frameBuffer")["size"].lastModified() >
          child("camera")["aspect"].lastCommitted()) {
        auto fbSize = child("frameBuffer")["size"].valueAs<vec2i>();
        child("camera")["aspect"] = fbSize.x / float(fbSize.y);
      }

      auto rHandle = child("renderer").valueAs<OSPRenderer>();
      auto cHandle = child("camera").valueAs<OSPCamera>();
      auto fHandle = child("frameBuffer").valueAs<OSPFrameBuffer>();

      bool newRenderer = currentRenderer != rHandle;
      bool newCamera = currentCamera != cHandle;
      bool newFB = currentFB != fHandle;

      if (newRenderer)
        currentRenderer = rHandle;

      if (newCamera)
        currentCamera = cHandle;

      if (newFB)
        currentFB = fHandle;

      if (newRenderer || newCamera) {
        ospSetObject(rHandle, "camera", cHandle);
        child("renderer").markAsModified(); // NOTE(jda) - neccessary??
      }

      bool rChanged = child("renderer").modifiedButNotCommitted();
      bool cChanged   = child("camera").modifiedButNotCommitted();

      clearFB = newFB || newCamera || newRenderer || rChanged || cChanged;
    }

    void Root::postCommit(RenderContext &ctx)
    {
      if (clearFB) {
        ospFrameBufferClear(
          child("frameBuffer").valueAs<OSPFrameBuffer>(),
          OSP_FB_COLOR | OSP_FB_ACCUM
        );

        clearFB = false;
      }
    }

    OSPPickResult Root::pick(const vec2f &pickPos)
    {
      auto rendererNode = child("renderer").nodeAs<Renderer>();
      return rendererNode->pick(pickPos);
    }

  } // ::ospray::sg
} // ::ospray
