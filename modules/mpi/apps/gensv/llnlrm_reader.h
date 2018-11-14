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

#include <cstdio>
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/FileName.h"
#include "ospcommon/vec.h"
#include "raw_reader.h"

namespace gensv {

class LLNLRMReader {
  ospcommon::FileName bobDir;
  int timestep;

public:
  // The directory format should be .*/bobNNN where NNN is the timestep
  LLNLRMReader(const ospcommon::FileName &bobDir);
  void loadBlock(const size_t blockID,
                 ospcommon::containers::AlignedVector<char> &data) const;
  static size_t numBlocks();
  static vec3sz blockGrid();
  static vec3sz blockSize();
  static vec3sz dimensions();
};

// we can also hard-code in to read the mesh the format for that
// is pretty straightforward.

}

