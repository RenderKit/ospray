// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include <ospcommon/vec.h>
#include <ospcommon/box.h>
#include <stdexcept>
#include <vector>

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {

  /*! though not required, it is good practice to put any module into
    its own namespace (isnide of ospray:: ). Unlike for the naming of
    library and init function, the naming for this namespace doesn't
    particularlly matter. E.g., 'bilinearPatch', 'module_blp',
    'bilinar_patch' etc would all work equally well. */
  namespace bilinearPatch {

    // use ospcommon for vec3f etc
    using namespace ospcommon;
    
    struct Patch {
      Patch(const vec3f &v00,
            const vec3f &v01,
            const vec3f &v10,
            const vec3f &v11)
        : v00(v00), v01(v01), v10(v10), v11(v11)
      {}
            
      vec3f v00, v01, v10, v11;
    };

    /*! parse a '.patch' file, and add its contents to the given list of
        patches */
    void readPatchesFromFile(std::vector<Patch> &patches,
                             const std::string &patchFileName);

    /*! create a list of patches from the list of given file names
        (each fiename is supposed to a patch file. if no patches could
        be read, create a simple test case. 'bounds' will be the world
        bouding box of all control points in the returned patch
        list */
    std::vector<Patch> readPatchesFromFiles(const std::vector<std::string> &fileNames,
                                            box3f &worldBounds);
    
  } // ::ospray::bilinearPatch
} // ::ospray
