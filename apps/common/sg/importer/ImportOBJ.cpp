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

#undef NDEBUG

// sg
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "../3rdParty/tiny_obj_loader.h"

#include <cstring>

namespace ospray {
  namespace sg {

    std::vector<std::shared_ptr<Material>>
    createSgMaterials(std::vector<tinyobj::material_t> &mats)
    {
      std::vector<std::shared_ptr<Material>> sgMaterials;

      for (auto &mat : mats) {
        auto matNodePtr = createNode(mat.name, "Material")->nodeAs<Material>();
        auto &matNode = *matNodePtr;

        auto type = mat.unknown_parameter["type"];
        if (!type.empty())
          matNode["type"].setValue(type);

        matNode["Ka"].setValue(vec3f(mat.ambient[0],
                                     mat.ambient[1],
                                     mat.ambient[2]));
        matNode["Kd"].setValue(vec3f(mat.diffuse[0],
                                     mat.diffuse[1],
                                     mat.diffuse[2]));
        matNode["Ks"].setValue(vec3f(mat.specular[0],
                                     mat.specular[1],
                                     mat.specular[2]));

        sgMaterials.push_back(matNodePtr);
      }

      return sgMaterials;
    }

    void importOBJ(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;

      std::string err;
      auto containingPath = fileName.path().str() + '/';
      bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                                  fileName.c_str(), containingPath.c_str());

      if (!err.empty())
        std::cerr << "#ospsg: obj parsing warning(s)...\n" << err << std::endl;

      if (!ret) {
        std::cerr << "#ospsg: FATAL error parsing obj file, no geometry added"
                  << " to the scene!" << std::endl;
        return;
      }

      auto sgMaterials = createSgMaterials(materials);

      auto v = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
      auto numSrcElements = attrib.vertices.size();
      v->v.resize(numSrcElements / 3);
      std::memcpy(v->v.data(), attrib.vertices.data(),
                  numSrcElements * sizeof(float));

      auto vn = createNode("normal", "DataVector3f")->nodeAs<DataVector3f>();
      numSrcElements = attrib.normals.size();
      vn->v.resize(numSrcElements / 3);
      std::memcpy(vn->v.data(), attrib.vertices.data(),
                  numSrcElements * sizeof(float));

      auto vt = createNode("texcoord", "DataVector2f")->nodeAs<DataVector2f>();
      numSrcElements = attrib.texcoords.size();
      vt->v.resize(numSrcElements / 2);
      std::memcpy(vt->v.data(), attrib.vertices.data(),
                  numSrcElements * sizeof(float));

      std::string base_name = fileName.name() + '_';
      int shapeId = 0;

      for (auto &shape : shapes) {
        auto name = base_name + std::to_string(shapeId++) + '_' + shape.name;
        auto mesh = createNode(name, "TriangleMesh")->nodeAs<TriangleMesh>();

        auto vi = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
        numSrcElements = shape.mesh.indices.size();
        vi->v.reserve(numSrcElements / 3);
        for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
          auto prim = vec3i(shape.mesh.indices[i+0].vertex_index,
                            shape.mesh.indices[i+1].vertex_index,
                            shape.mesh.indices[i+2].vertex_index);
          vi->push_back(prim);
        }

        mesh->add(v);
        mesh->add(vn);
        mesh->add(vt);
        mesh->add(vi);

        auto matIdx = shape.mesh.material_ids[0];
        if (!sgMaterials.empty() && matIdx > 0)
          mesh->setChild("material", sgMaterials[matIdx]);

        auto model = createNode(name + " model", "Model");
        model->add(mesh);

        auto instance = createNode(name + " instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
      }
    }

  } // ::ospray::sg
} // ::ospray

