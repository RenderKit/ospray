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

#pragma once

#include "ospcommon/FileName.h"
#include "ospcommon/vec.h"
#include <cstdio>

namespace gensv {

using vec3sz = ospcommon::vec_t<size_t, 3>;

// Convenience class for reading/seeking around RAW volume files
class RawReader {
  ospcommon::FileName fileName;
  vec3sz dimensions;
  size_t voxelSize;
  FILE *file;
  int64_t offset;

public:
  RawReader(const ospcommon::FileName &fileName,
      const vec3sz &dimensions, size_t voxelSize);
  ~RawReader();
  // Read a region of volume data from the file into the buffer passed.
  // It's assumed the buffer passed has enough room. Returns the
  // number voxels read
  size_t readRegion(const vec3sz &start,
      const vec3sz &size, unsigned char *buffer);

private:
  inline bool convexRead(const vec3sz &size) {
    // 3 cases for convex reads:
    // - We're reading a set of slices of the volume
    // - We're reading a set of scanlines of a slice
    // - We're reading a set of voxels in a scanline
    return (size.x == dimensions.x && size.y == dimensions.y)
      || (size.x == dimensions.x && size.z == 1)
      || (size.y == 1 && size.z == 1);
  }
};

}

