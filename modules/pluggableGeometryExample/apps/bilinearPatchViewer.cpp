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

#include <ospray/ospray.h>
#include <ospcommon/vec.h>
#include <ospcommon/box.h>
#include <stdexcept>

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
    
    /*! helper class to parse command-line arguments */
    struct CommandLine {
      CommandLine(int ac, const char **av);
      std::vector<std::string> inputFiles;
    };

    CommandLine::CommandLine(int ac, const char **av)
    {
      for (int i=1;i<ac;i++) {
        const std::string arg = av[i];
        if (arg[0] == '-') {
          throw std::runtime_error("un-handled cmdline argument '"+arg+"'");
        } else {
          // no arg: must be an input file
          inputFiles.push_back(arg);
        }
      }
    }
    
    extern "C" int main(int ac, const char **av)
    {
      // initialize ospray (this also takes all ospray-related args
      // off the command-line)
      ospInit(&ac,av);
      
      // parse the commandline; complain about anything we do not
      // recognize
      CommandLine args(ac,av);
      if (args.inputFiles.empty())
        throw std::runtime_error("no input files specified");

      // import the patches from the sample files
      std::vector<Patch> patches;
      for (auto fileName : args.inputFiles)
        readPatchesFromFile(patches,fileName);

      box3f bounds = empty;
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


      // create the actual viewer ....
      throw std::runtime_error("creating actual viewer not implemented yet ...");
    }
        
  } // ::ospray::bilinearPatch
} // ::ospray
