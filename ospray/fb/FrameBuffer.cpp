// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "FrameBuffer.h"
#include "FrameBuffer_ispc.h"
#include "LocalFB_ispc.h"

namespace ospray {

  FrameBuffer::FrameBuffer(const vec2i &size,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer)
    : size(size),
      colorBufferFormat(colorBufferFormat),
      hasDepthBuffer(hasDepthBuffer),
      hasAccumBuffer(hasAccumBuffer),
      accumID(-1)
  {
    managedObjectType = OSP_FRAMEBUFFER;
    Assert(size.x > 0 && size.y > 0);
  }

  void FrameBuffer::commit()
  {
    const float gamma = getParam1f("gamma", 1.0f);
    ispc::FrameBuffer_set_gamma(ispcEquivalent, gamma);
  }

  /*! helper function for debugging. write out given pixels in PPM format */
  void writePPM(const std::string &fileName, const vec2i &size, uint32 *pixels)
  {
    FILE *file = fopen(fileName.c_str(),"w");
    if (!file) {
      std::cout << "#osp:fb: could not open file " << fileName << std::endl;
      return;
    }
    fprintf(file,"P6\n%i %i\n255\n",size.x,size.y);
    for (int i=0;i<size.x*size.y;i++) {
      char *ptr = (char*)&pixels[i];
      fwrite(ptr,1,3,file);
    }
    fclose(file);
  }

} // ::ospray
