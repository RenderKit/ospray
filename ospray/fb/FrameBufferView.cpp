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

#include "FrameBufferView.h"
#include "FrameBuffer.h"

namespace ospray {

  FrameBufferView::FrameBufferView(FrameBuffer *fb,
                                   OSPFrameBufferFormat colorFormat,
                                   void *colorBuffer,
                                   float *depthBuffer,
                                   vec3f *normalBuffer,
                                   vec3f *albedoBuffer)
      : fbDims(fb->getNumPixels()),
        viewDims(fbDims),
        haloDims(0),
        colorBufferFormat(colorFormat),
        colorBuffer(colorBuffer),
        depthBuffer(depthBuffer),
        normalBuffer(normalBuffer),
        albedoBuffer(albedoBuffer)
  {
  }

}  // namespace ospray
