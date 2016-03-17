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
#include <fstream>

namespace ospray {
  namespace miniSG {
    using std::cout;
    using std::endl;

    /*! import a HBP file, and add it to the specified model */
    void importHBP(Model &model, const ospcommon::FileName &fileName)
    {
      std::string vtxName = fileName.str()+".vtx";
      std::string triName = fileName.str()+".tri";
      FILE *vtx = fopen(vtxName.c_str(),"rb");
      FILE *tri = fopen(triName.c_str(),"rb");

      Mesh *mesh = new Mesh;
      vec3f v; 
      Triangle t;

      while (fread(&v,sizeof(v),1,vtx))
        mesh->position.push_back(vec3fa(v));
      while (fread(&t,sizeof(t),1,tri))
        mesh->triangle.push_back(t);
      
      model.mesh.push_back(mesh);
      model.instance.push_back(Instance(model.mesh.size()-1));
      mesh->material = NULL; 
    }

  } // ::ospray::minisg
} // ::ospray
