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
      const char *fileName = strstr(url.str().c_str(),"://")+3;
      FILE *file = fopen(fileName,"rb");
      if (!file)
        throw std::runtime_error("could not open file "+std::string(fileName));

      // read the data vector
      std::shared_ptr<DataVectorT<Spheres::Sphere,OSP_RAW>> data
        = std::make_shared<DataVectorT<Spheres::Sphere,OSP_RAW>>();
      
      float f[5];
      while (fread(f,sizeof(float),5,file) == 5) {
        data->v.push_back(Spheres::Sphere(vec3f(f[0],f[1],f[2]),.1,0));
      }
      fclose(file);

      
      // create the node
      NodeHandle spheres = createNode("spheres","Spheres");
      spheres["data"]->setValue(NodeHandle(data));

      NodeHandle(world) += spheres;
    }

  }// ::ospray::sg
}// ::ospray


