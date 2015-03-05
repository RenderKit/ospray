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

#include "importer.h"

namespace ospray {
  namespace miniSG {

    ImportHelper::ImportHelper(Model &model, const std::string &name)
      : model(&model) 
    {
      mesh = new Mesh;
      mesh->bounds = embree::empty;
    }

    void ImportHelper::finalize()
    {
      Assert(mesh);
      const int meshID = model->mesh.size();
      model->mesh.push_back(mesh);
      model->instance.push_back(Instance(meshID));
      mesh = NULL;
      known_positions.clear();
      known_normals.clear();
      known_texcoords.clear();
    }

    /*! find given vertex and return its ID, or add if it doesn't yet exist */
    uint32 ImportHelper::addVertex(const vec3f &position)
    {
      Assert(mesh);
      if (known_positions.find(position) == known_positions.end()) {
        int newID = mesh->position.size();
        mesh->position.push_back(position);
        mesh->bounds.extend(position);
        known_positions[position] = newID;
        return newID;
      } else
        return known_positions[position];
    }
    
    /*! add new triangle to the mesh. may discard the triangle if it is degenerated. */
    void ImportHelper::addTriangle(const miniSG::Triangle &triangle)
    {
      Assert(mesh);
      mesh->triangle.push_back(triangle);
    }

  } // ::ospray::minisg
} // ::ospray
