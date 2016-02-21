// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

    void importTRI(Model &model,
                   const embree::FileName &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"rb");
      if (!file) error("could not open input file");

      int32 numVertices;
      fread(&numVertices,1,sizeof(numVertices),file);

      Mesh *mesh = new Mesh;
      model.mesh.push_back(mesh);

      mesh->position.resize(numVertices);
      mesh->normal.resize(numVertices);
      mesh->triangle.resize(numVertices/3);
      fread(&mesh->position[0],numVertices,4*sizeof(float),file);
      fread(&mesh->normal[0],numVertices,4*sizeof(float),file);
      for (int i=0;i<numVertices/3;i++) {
        mesh->triangle[i].v0 = 3*i+0;
        mesh->triangle[i].v1 = 3*i+1;
        mesh->triangle[i].v2 = 3*i+2;
      }
      model.instance.push_back(Instance(0));
    }

  } // ::ospray::minisg
} // ::ospray


