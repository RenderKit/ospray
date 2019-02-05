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

#include <string>
#include <stdio.h>
#include <errno.h>

#include "../memory/malloc.h"
#include "../vec.h"

namespace ospcommon {
  namespace utility {

    template <typename COMP_T, int N_COMP, typename PIXEL_T, int PIXEL_COMP,
             bool FLIP>
    inline void writeImage(const std::string &fileName,
          const char *const header,
          const int sizeX, const int sizeY,
          const PIXEL_T *const pixel)
    {
      FILE *file = fopen(fileName.c_str(), "wb");
      if (file == nullptr)
        throw std::runtime_error("Can't open file for writeP[FP]M!");

      fprintf(file, header, sizeX, sizeY);
      auto out = STACK_BUFFER(COMP_T, N_COMP*sizeX);
      for (int y = 0; y < sizeY; y++) {
        auto *in = (const COMP_T*)&pixel[(FLIP?sizeY-1-y:y)*sizeX];
        for (int x = 0; x < sizeX; x++)
          for (int c = 0; c < N_COMP; c++)
            out[N_COMP*x + c] = in[PIXEL_COMP*x + (N_COMP==1?3:c)];
        fwrite(out, N_COMP*sizeX, sizeof(COMP_T), file);
      }
      fprintf(file, "\n");
      fclose(file);
    }

    inline void writePPM(const std::string &fileName,
                         const int sizeX, const int sizeY,
                         const uint32_t *pixel)
    {
      writeImage<unsigned char, 3, uint32_t, 4, true>(fileName,
          "P6\n%i %i\n255\n", sizeX, sizeY, pixel);
    }

    inline void writePGM(const std::string &fileName,
                         const int sizeX, const int sizeY,
                         const uint32_t *pixel)
    {
      writeImage<unsigned char, 1, uint32_t, 4, true>(fileName,
          "P5\n%i %i\n255\n", sizeX, sizeY, pixel);
    }

    template <typename T>
    inline void writePFM(const std::string &fName,
                         const int sizeX, const int sizeY,
                         const T *p)
    {
      static_assert(std::is_same<T,vec4f>::value||std::is_same<T,vec3fa>::value
          ||std::is_same<T,vec3f>::value,
          "writePFM needs pixels as vec3f(a)* or vec4f*");
      writeImage<float, 3, T, std::is_same<T,vec3f>::value? 3:4, false>(fName,
          "PF\n%i %i\n-1.0\n", sizeX, sizeY, p);
    }

  } // ::ospcommon::utility
} // ::ospcommon
