// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

/*! \file importer/SVC.cpp Scene Graph Importer Plugin for Neuromorpho SWC files */

// ospray
// ospray/sg
#include "sg/module/Module.h"
#include "sg/importer/Importer.h"
// this module
#include "../geometry/SWC.h"
// stl
#include <set>

namespace ospray {
  namespace sg {

    inline std::ostream &operator<<(std::ostream &o, const SWCGeometry::Node &n)
    { o << n.pos << ":" << n.rad; return o; }

    void importSLRAW(const FileName &fileName,
                   ImportState &state)
    {
      std::cout << "#osp:sg:neuron: Importing SLRAW file '" << fileName << "'" << std::endl;
      FILE *file = fopen(fileName.str().c_str(),"r");
      if (!file)
        throw RuntimeError("could not open file '"+fileName.str()+"'");

#if SINGLE_SWC_GEOMETRY
      static SWCGeometry *globalSWC = NULL;
      SWCGeometry *swc = globalSWC; 
      if (swc == NULL) {
      swc = globalSWC = new SWCGeometry;
    }
#else
      SWCGeometry *swc = new SWCGeometry;
#endif
      swc->checkData = false;

      float radius = 0.1f;
      char *radFromEnv = getenv("SLRAW_RADIUS");
      if (radFromEnv)
        radius = atof(radFromEnv);
      try {
        int32 numStreamLines;
        fread(&numStreamLines,sizeof(numStreamLines),1,file);
        PRINT(numStreamLines);

        swc->startIndexVec.push_back(std::pair<int,int>(swc->nodeVec.size(),swc->linkVec.size()));

        std::map<int,int> index;
        for (size_t slID=0;slID<numStreamLines;slID++) {
          int32 numPoints;
          fread(&numPoints,sizeof(numPoints),1,file);
          for (size_t pointID=0;pointID<numPoints;pointID++) {
            vec3f p;
            fread(&p,sizeof(p),1,file);
            swc->nodeVec.push_back(SWCGeometry::Node(p,radius));
            if (pointID > 0)
              swc->linkVec.push_back(std::pair<int,int>(swc->nodeVec.size()-1,swc->nodeVec.size()-2));
          }
        }
#if SINGLE_SWC_GEOMETRY
        static bool alreadyAdded = 0;
        if (!alreadyAdded) {
          alreadyAdded = true;
          state.world->add(swc);
        }
#else
        state.world->add(swc);
#endif
      }
      catch (std::runtime_error e) {
        delete swc;
        fclose(file);
        throw e;
      }

      fclose(file);
    }

  }
}

