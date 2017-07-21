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

#include "../malloc.h"

namespace ospcommon {
  namespace utility {

    inline void writePPM(const std::string &fileName,
                         const int sizeX, const int sizeY,
                         const uint32_t *pixel)
    {
      FILE *file = fopen(fileName.c_str(), "wb");
      fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
      unsigned char *out = (unsigned char *)alloca(3*sizeX);
      for (int y = 0; y < sizeY; y++) {
        auto *in = (const unsigned char *)&pixel[(sizeY-1-y)*sizeX];
        for (int x = 0; x < sizeX; x++) {
          out[3*x + 0] = in[4*x + 0];
          out[3*x + 1] = in[4*x + 1];
          out[3*x + 2] = in[4*x + 2];
        }
        fwrite(out, 3*sizeX, sizeof(char), file);
      }
      fprintf(file, "\n");
      fclose(file);
    }

  } // ::ospcommon::utility
} // ::ospcommon