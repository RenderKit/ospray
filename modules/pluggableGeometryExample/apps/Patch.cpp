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
    
    /*! create a list of patches from the list of given file names
        (each fiename is supposed to a patch file. if no patches could
        be read, create a simple test case. 'bounds' will be the world
        bouding box of all control points in the returned patch
        list */
    std::vector<Patch> readPatchesFromFiles(const std::vector<std::string> &fileNames,
                                            box3f &bounds)
    {
      std::vector<Patch> patches;
      for (auto fileName : fileNames) 
        readPatchesFromFile(patches,fileName);

      if (patches.empty()) {
        std::cout << "#osp.blp: no input files specified - creating default path" << std::endl;
        patches.push_back(Patch(vec3f(0.f,1.f,0.f),
                                vec3f(0.f,0.f,1.f),
                                vec3f(1.f,0.f,0.f),
                                vec3f(1.f,1.f,1.f)));
      }

      bounds = empty;
      for (auto patch : patches) {
        bounds.extend(patch.v00);
        bounds.extend(patch.v01);
        bounds.extend(patch.v10);
        bounds.extend(patch.v11);
      }
      std::cout << "##################################################################" << std::endl;
      std::cout << "#osp:blp: done parsing input files" << std::endl;
      std::cout << "#osp:blp: found a total of  " << patches.size() << " patches ..." << std::endl;
      std::cout << "#osp:blp: ... with world bounds of " << bounds << std::endl;
      std::cout << "##################################################################" << std::endl;
      return patches;
    }
    
  } // ::ospray::bilinearPatch
} // ::ospray
