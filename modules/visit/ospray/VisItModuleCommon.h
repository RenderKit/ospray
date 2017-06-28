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

#pragma once

#include "fb/Tile.h"
// this file will be installed so can expose new functions to the users

namespace ospray {
    namespace visit {

	// this is a place to define all global flags that might be helpful
	// throughout the program

	// definition of tiles for MPI distributed renderer
	struct TileInfo 
	{
	    // basic info
	    int regionSize[4] = {0};
	    int fbSize[2] = {0};
	    float depth{std::numeric_limits<float>::infinity()};
	    bool visible{false};
            // 'red' component; in float.
	    float r[TILE_SIZE*TILE_SIZE];
	    // 'green' component; in float.
	    float g[TILE_SIZE*TILE_SIZE];
	    // 'blue' component; in float.
	    float b[TILE_SIZE*TILE_SIZE];
	    // 'alpha' component; in float.
	    float a[TILE_SIZE*TILE_SIZE];
	    // constructor
	    ~TileInfo() = default;
	    TileInfo() = default;
	    TileInfo(const Tile& src);
	};
	typedef std::vector<std::vector<TileInfo>> TileRegionList;
	struct TileRetriever 
	{
	    virtual void operator()(const TileRegionList& list) {}
	};

	//! this is the verbose flag, which is used in VisIt also
	extern bool verbose;
	inline bool CheckVerbose() { return verbose; }

    };
};
