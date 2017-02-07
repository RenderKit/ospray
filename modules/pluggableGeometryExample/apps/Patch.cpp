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

#include "Patch.h"

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
    
    /*! parse a '.patch' file, and add its contents to the given list of
        patches */
    void readPatchesFromFile(std::vector<Patch> &patches,
                             const std::string &patchFileName)
    {
      FILE *file = fopen(patchFileName.c_str(),"r");
      if (!file)
        throw std::runtime_error("could not open input file '"+patchFileName+"'");

      std::vector<vec3f> parsedPoints;

      size_t numPatchesRead = 0;
      static const size_t lineSize = 10000;
      char line[lineSize];
      while (fgets(line,10000,file)) {
        // try to parse three floats...
        vec3f p;
        int rc = sscanf(line,"%f %f %f",&p.x,&p.y,&p.z);
        if (rc != 3)
          // could not read a point - must be a empty or comment line; just ignore
          continue;

        // add this point to list of parsed points ...
        parsedPoints.push_back(p);

        // ... and if we have four of them, we have a patch!
        if (parsedPoints.size() == 4) {
          patches.push_back(Patch(parsedPoints[0],parsedPoints[1],
                                  parsedPoints[2],parsedPoints[3]));
          parsedPoints.clear();
          ++numPatchesRead;
        }
      }
      std::cout << "#osp:blp: done parsing " << patchFileName
                << " (" << numPatchesRead << " patches)" << std::endl;
    }
    
    
  } // ::ospray::bilinearPatch
} // ::ospray
