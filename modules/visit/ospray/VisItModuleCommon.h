// ======================================================================== //
// Copyright Qi WU                                                          //
//   Scientific Computing and Image Institution                             //
//   University of Utah                                                     //
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
// this file will be installed so can expose new functions to the users

#pragma once

#include <vector>
#include <limits>
#include <cstddef>

namespace ospray {
  namespace visit {
    //
    // this is a place to define all global flags that might be helpful
    // throughout the program
    //
    //! tile type for MPI renderer
    struct VisItTile {
      // basic info
      int region[4];
      int fbSize[2];
      float minDepth;
      float maxDepth;
      // 'red'   component in float.
      std::vector<float> r;
      // 'green' component in float.
      std::vector<float> g;
      // 'blue'  component in float.
      std::vector<float> b;
      // 'alpha' component in float.
      std::vector<float> a;
      // constructor
      virtual ~VisItTile() = default;
      VisItTile() {
	region[0] = region[1] = region[2] = region[3] = 0;
	fbSize[0] = fbSize[1] = 0;
	minDepth = std::numeric_limits<float>::max();
	maxDepth = std::numeric_limits<float>::min();
      };
    };
    typedef std::vector<VisItTile*> VisItTileArray;
    struct VisItTileRetriever 
    {
      virtual void operator()(const VisItTileArray& list) = 0;
    };
    //
    //! this is the verbose flag, which is used in VisIt also
    extern bool verbose;
    inline bool CheckVerbose() { return verbose; }
  };
};
