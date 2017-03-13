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

#include "Importer.h"
#include "common/sg/SceneGraph.h"
#include <memory>

/*! \file sg/module/Importer.cpp Defines the interface for writing
    file importers for the ospray::sg */

namespace ospray {
  namespace sg {

    // for now, let's hardcode the importers - should be moved to a
    // registry at some point ...
    void importFileType_points(std::shared_ptr<World> &world,
                               const FileName &url)
    {
      std::cout << "--------------------------------------------" << std::endl;
      std::cout << "#osp.sg: importer for 'points': " << url.str() << std::endl;

      FormatURL fu(url.str());
      PRINT(fu.fileName);

      // const std::string portHeader = strstr(url.str().c_str(),"://")+3;
      
      // const char *fileName = strstr(url.str().c_str(),"://")+3;
      PRINT(fu.fileName);
      FILE *file = fopen(fu.fileName.c_str(),"rb");
      if (!file)
        throw std::runtime_error("could not open file "+fu.fileName);

      // read the data vector
      PING;
      std::shared_ptr<DataVectorT<Spheres::Sphere,OSP_RAW>> data
        = std::make_shared<DataVectorT<Spheres::Sphere,OSP_RAW>>();

      if (!fu.hasArg("format"))
        throw std::runtime_error("'points' importer error: 'format=...' not specified...");

      if (fu["format"] == "xyzr") {
        float f[4];
        while (fread(f,sizeof(float),4,file) == 4) {
          data->v.push_back(Spheres::Sphere(vec3f(f[0],f[1],f[2]),f[3],0));
        }
      }
      else throw std::runtime_error("#osp.sg.importPoints: unknown format '"+fu["format"]+"'");
      fclose(file);

      PING;
      
      // create the node
      NodeHandle spheres = createNode("spheres","Spheres");

      PING;
      // iw - note that 'add' sounds wrong here, but that's the way
      // the current scene graph works - 'adding' that node (which
      // happens to have the right name) will essentially replace the
      // old value of that node, and thereby assign the 'data' field
      data->setName("data");
      spheres->add(data); //["data"]->setValue(data);
      PING;

      NodeHandle(world) += spheres;
    }

  }// ::ospray::sg
}// ::ospray


