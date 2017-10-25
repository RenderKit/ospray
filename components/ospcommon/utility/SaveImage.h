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

#include <string>
#include <stdio.h>
#include <errno.h>

#include "../malloc.h"
#include "../vec.h"

namespace ospcommon {
  namespace utility {

    template <typename COMP_T, typename PIXEL_T, bool flip>
    inline void writeImage(const std::string &fileName,
          const char *const header,
          const int sizeX, const int sizeY,
          const PIXEL_T *const pixel)
    {
      FILE *file = fopen(fileName.c_str(), "wb");
      if (file == nullptr)
        throw std::runtime_error("Can't open file for writeP[FP]M!");

      fprintf(file, header, sizeX, sizeY);
      auto out = STACK_BUFFER(COMP_T, 3*sizeX);
      for (int y = 0; y < sizeY; y++) {
        auto *in = (const COMP_T*)&pixel[(flip?sizeY-1-y:y)*sizeX];
        for (int x = 0; x < sizeX; x++) {
          out[3*x + 0] = in[4*x + 0];
          out[3*x + 1] = in[4*x + 1];
          out[3*x + 2] = in[4*x + 2];
        }
        fwrite(out, 3*sizeX, sizeof(COMP_T), file);
      }
      fprintf(file, "\n");
      fclose(file);
    }

    inline void writePPM(const std::string &fileName,
                         const int sizeX, const int sizeY,
                         const uint32_t *pixel)
    {
      writeImage<unsigned char, uint32_t, true>(fileName, "P6\n%i %i\n255\n",
          sizeX, sizeY, pixel);
    }

    template <typename T>
    inline void writePFM(const std::string &fName,
                         const int sizeX, const int sizeY,
                         const T *p)
    {
      static_assert(std::is_same<T,vec4f>::value||std::is_same<T,vec3fa>::value,
          "writePFM needs pixels as vec3fa* or vec4f*");
      writeImage<float, T, false>(fName, "PF\n%i %i\n-1.0\n", sizeX, sizeY, p);
    }

  } // ::ospcommon::utility
} // ::ospcommon
