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

#include "miniSG.h"
#include "importer.h"

namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;

    /*! NASA 'tri' format, with three vec3f vertices per triangle (no
        indices in file) */
    void importTRI_xyz(Model &model,
                   const ospcommon::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file) error("could not open input file");

      Mesh *mesh = new Mesh;

      vec3f tri[3];
      while (fread(&tri,sizeof(vec3f),3,file) == 3) {
        mesh->position.push_back(tri[0]);
        mesh->position.push_back(tri[1]);
        mesh->position.push_back(tri[2]);
      }
      for (size_t i = 0; i < mesh->position.size()/3; i++) {
        mesh->triangle.push_back(Triangle(vec3i(3*i)+vec3i(0,1,2)));
      }
      mesh->material = new Material;
      mesh->material->name = "OBJ";
      mesh->material->setParam("Kd", vec3f(.7f));
      
      model.instance.push_back(Instance(model.mesh.size()));
      model.mesh.push_back(mesh);
      std::cout << "#msg: loaded .tri file of " << mesh->triangle.size()
                << " triangles" << std::endl;
    }

    /*! NASA 'tri' format, with three vec3fa vertices per triangle (no
        indices in file); fourth component per vertex is typically a
        scalar used for coloring - we ignore that for now */
    void importTRI_xyzs(Model &model,
                   const ospcommon::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file) error("could not open input file");

      Mesh *mesh = new Mesh;

      vec3fa tri[3];
      while (fread(&tri,sizeof(vec3f),3,file) == 3) {
        mesh->position.push_back(tri[0]);
        mesh->position.push_back(tri[1]);
        mesh->position.push_back(tri[2]);
      }
      for (size_t i = 0; i < mesh->position.size()/3; i++) {
        mesh->triangle.push_back(Triangle(vec3i(3*i)+vec3i(0,1,2)));
      }

      mesh->material = new Material;
      mesh->material->name = "OBJ";
      mesh->material->setParam("Kd", vec3f(.7f));

      model.instance.push_back(Instance(model.mesh.size()));
      model.mesh.push_back(mesh);
      std::cout << "#msg: loaded .tri file of " << mesh->triangle.size()
                << " triangles" << std::endl;
    }

  } // ::ospray::minisg
} // ::ospray


