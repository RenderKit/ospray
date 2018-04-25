// ======================================================================== //
// Copyright 2017-2018 Intel Corporation                                    //
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

#include <cassert>
#include <stdexcept>
#include "raw_reader.h"

using namespace ospcommon;

namespace gensv {

RawReader::RawReader(const FileName &fileName, const vec3sz &dimensions, size_t voxelSize)
  : fileName(fileName), dimensions(dimensions), voxelSize(voxelSize),
  file(fopen(fileName.c_str(), "rb")), offset(0)
{
  if (!file) {
    throw std::runtime_error("ImportRAW: Unable to open file " + fileName.str());
  }
}
RawReader::~RawReader() {
  fclose(file);
}
// Read a region of volume data from the file into the buffer passed. It's assumed
// the buffer passed has enough room. Returns the number voxels read
size_t RawReader::readRegion(const vec3sz &start, const vec3sz &size, unsigned char *buffer) {
  assert(size.x > 0 && size.y > 0 && size.z > 0);

  int64_t startRead = (start.x + dimensions.x * (start.y + dimensions.y * start.z)) * voxelSize;
  if (offset != startRead) {
    int64_t seekAmt = startRead - offset;
    if (fseek(file, seekAmt, SEEK_CUR) != 0) {
      throw std::runtime_error("ImportRAW: Error seeking file");
    }
    offset = startRead;
  }

  // Figure out how to actually read the region since it may not be a full X/Y slice and
  // we'll need to read the portions in X & Y and seek around to skip regions
  size_t read = 0;
  if (convexRead(size)) {
    read = fread(buffer, voxelSize, size.x * size.y * size.z, file);
    offset = startRead + read * voxelSize;
  } else if (size.x == dimensions.x) {
    for (size_t z = start.z; z < start.z + size.z; ++z) {
      const vec3sz startSlice(start.x, start.y, z);
      const vec3sz sizeSlice(size.x, size.y, 1);
      read += readRegion(startSlice, sizeSlice, buffer + read * voxelSize);
    }
  } else {
    for (size_t z = start.z; z < start.z + size.z; ++z) {
      for (size_t y = start.y; y < start.y + size.y; ++y) {
        const vec3sz startLine(start.x, y, z);
        const vec3sz sizeLine(size.x, 1, 1);
        read += readRegion(startLine, sizeLine, buffer + read * voxelSize);
      }
    }
  }
  return read;
}

}

