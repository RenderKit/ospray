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

      float radius = .1f;
      if (fu.hasArg("radius"))
        radius = std::stof(fu["radius"]);
      if (radius == 0.f)
        throw std::runtime_error("#sg.importPoints: could not parse radius ...");
      
      std::string format = "xyz";
      if (fu.hasArg("format"))
        format = fu["format"];

      /* for now, hard-coded sphere componetns to be in float format,
         so the number of chars in the format string is the num components */
      int numFloatsPerSphere = format.size();
      size_t xPos = format.find("x");
      size_t yPos = format.find("y");
      size_t zPos = format.find("z");
      size_t rPos = format.find("r");
      size_t sPos = format.find("s");

      if (xPos == std::string::npos)
        throw std::runtime_error("invalid points format: no x component");
      if (yPos == std::string::npos)
        throw std::runtime_error("invalid points format: no y component");
      if (zPos == std::string::npos)
        throw std::runtime_error("invalid points format: no z component");

      float f[numFloatsPerSphere];
      while (fread(f,sizeof(float),numFloatsPerSphere,file) == numFloatsPerSphere) {
        // read one more sphere ....
        Spheres::Sphere s;
        s.position.x = f[xPos];
        s.position.y = f[yPos];
        s.position.z = f[zPos];
        s.radius
          = (rPos == std::string::npos)
          ? radius
          : f[rPos];
        data->v.push_back(s);
      }
      fclose(file);

      // create the node
      NodeHandle spheres = createNode("spheres","Spheres");

      // iw - note that 'add' sounds wrong here, but that's the way
      // the current scene graph works - 'adding' that node (which
      // happens to have the right name) will essentially replace the
      // old value of that node, and thereby assign the 'data' field
      data->setName("data");
      spheres->add(data); //["data"]->setValue(data);

      NodeHandle(world) += spheres;
    }

  }// ::ospray::sg
}// ::ospray


