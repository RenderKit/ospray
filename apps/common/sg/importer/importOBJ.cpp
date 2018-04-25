// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#define TINYOBJLOADER_IMPLEMENTATION  // define this in only *one* .cc
#include "../3rdParty/tiny_obj_loader.h"

#include <cstring>
#include <sstream>

#define USE_INSTANCES 0

namespace ospray {
  namespace sg {

    std::shared_ptr<Texture2D> loadTexture(const FileName &fullPath,
                                           const bool preferLinear = false)
    {
      std::shared_ptr<Texture2D> tex = Texture2D::load(fullPath, preferLinear);
      if (!tex)
        std::cout << "could not load texture " << fullPath.str() << " !\n";

      return tex;
    }

    void addTextureIfNeeded(Material &node,
                            const std::string &name,
                            const FileName &texName,
                            const FileName &containingPath,
                            bool preferLinear = false)
    {
      if (!texName.str().empty()) {
        auto tex = loadTexture(containingPath + texName, preferLinear);
        if (tex) {
          tex->setName(name);
          node.setChild(name, tex);
        }
      }
    }

    static inline void parseParameterString(std::string typeAndValueString,
                                            std::string &paramType,
                                            ospcommon::utility::Any &paramValue)
    {
      std::stringstream typeAndValueStream(typeAndValueString);
      std::string paramValueString;
      getline(typeAndValueStream, paramValueString);

      std::vector<float> floats;
      std::stringstream valueStream(typeAndValueString);
      float val;
      while (valueStream >> val)
        floats.push_back(val);

      if (floats.size() == 1) {
        paramType = "float";
        paramValue = floats[0];
      } else if (floats.size() == 2) {
        paramType = "vec2f";
        paramValue = vec2f(floats[0], floats[1]);
      } else if (floats.size() == 3) {
        paramType = "vec3f";
        paramValue = vec3f(floats[0], floats[1], floats[2]);
      } else {
        // Unknown type.
        paramValue = typeAndValueString;
      }
    }

    static inline std::shared_ptr<MaterialList> createSgMaterials(
        std::vector<tinyobj::material_t> &mats, const FileName &containingPath)
    {
      auto sgMaterials =
          createNode("materialList", "MaterialList")->nodeAs<MaterialList>();

      if (mats.empty()) {
        sgMaterials->push_back(createNode("default", "Material")->nodeAs<Material>());
        return sgMaterials;
      }

      for (auto &mat : mats) {
        auto matNodePtr = createNode(mat.name, "Material")->nodeAs<Material>();
        auto &matNode   = *matNodePtr;
        bool addOBJparams = true;

        for (auto &param : mat.unknown_parameter) {
          if (param.first == "type") {
            matNode["type"] = param.second;
            if (param.second != "OBJMaterial" && param.second != "default")
              addOBJparams = false;
          } else {
            std::string paramType;
            ospcommon::utility::Any paramValue;
            if (param.first.find("Map") != std::string::npos)
            {
              addTextureIfNeeded(matNode, param.first,
                                 param.second, containingPath);
            }
            else
            {
              parseParameterString(param.second, paramType, paramValue);
              try {
                matNode.createChildWithValue(param.first, paramType, paramValue);
              } catch (const std::runtime_error &) {
                // NOTE(jda) - silently move on if parsed node type doesn't exist
                // maybe it's a texture, try it
                std::cout << "attempting to load param as texture: "
                          << param.first << " " << param.second << std::endl;
                addTextureIfNeeded(matNode, param.first,
                                   param.second, containingPath);
              }
            }
          }
        }

        if (addOBJparams) {
          matNode["d"]  = mat.dissolve;
          matNode["Kd"] = vec3f(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
          matNode["Ks"] =
            vec3f(mat.specular[0], mat.specular[1], mat.specular[2]);

          addTextureIfNeeded(
              matNode, "map_Kd", mat.diffuse_texname, containingPath);
          addTextureIfNeeded(
              matNode, "map_Ks", mat.specular_texname, containingPath);
          addTextureIfNeeded(matNode,
              "map_Ns",
              mat.specular_highlight_texname,
              containingPath,
              true);
          addTextureIfNeeded(
              matNode, "map_bump", mat.bump_texname, containingPath);
          addTextureIfNeeded(
              matNode, "map_d", mat.alpha_texname, containingPath, true);
        }

        sgMaterials->push_back(matNodePtr);
      }

      return sgMaterials;
    }

    void importOBJ(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;

      std::string err;
      const std::string containingPath = fileName.path();

      std::cout << "parsing OBJ input file... \n";
      bool ret = tinyobj::LoadObj(&attrib,
                                  &shapes,
                                  &materials,
                                  &err,
                                  fileName.c_str(),
                                  containingPath.c_str());

#if 0 // NOTE(jda) - enable if you want to see warnings from TinyOBJ
      if (!err.empty())
        std::cerr << "#ospsg: obj parsing warning(s)...\n" << err << std::endl;
#endif

      if (!ret) {
        std::cerr << "#ospsg: FATAL error parsing obj file, no geometry added"
                  << " to the scene!" << std::endl;
        return;
      }

      auto sgMaterials = createSgMaterials(materials, containingPath);

      std::string base_name = fileName.name() + '_';
      int shapeId           = 0;

      std::cout << "...adding found triangle groups to the scene...\n";

      size_t shapeCounter = 0;
      size_t numShapes    = shapes.size();
      size_t increment    = numShapes / size_t(10);
      int    incrementer  = 0;

#if !USE_INSTANCES
      auto objInstance = createNode("instance", "Instance");
      world->add(objInstance);
#endif
      for (auto &shape : shapes) {
        for (int numVertsInFace : shape.mesh.num_face_vertices) {
          if (numVertsInFace != 3) {
            std::cerr << "Warning: more thant 3 verts in face!";
            PRINT(numVertsInFace);
          }
        }

        if (shapeCounter++ > (increment * incrementer + 1))
          std::cout << incrementer++ * 10 << "%\n";

        auto name = base_name + std::to_string(shapeId++) + '_' + shape.name;
        auto mesh = createNode(name, "TriangleMesh")->nodeAs<TriangleMesh>();

        auto v = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        auto numSrcIndices = shape.mesh.indices.size();
        v->v.reserve(numSrcIndices);

        auto vi = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
        vi->v.reserve(numSrcIndices / 3);

        auto vn = createNode("normal", "DataVector3f")->nodeAs<DataVector3f>();
        vn->v.reserve(numSrcIndices);

        auto vt =
            createNode("texcoord", "DataVector2f")->nodeAs<DataVector2f>();
        vt->v.reserve(numSrcIndices);

        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
          auto idx0 = shape.mesh.indices[i + 0];
          auto idx1 = shape.mesh.indices[i + 1];
          auto idx2 = shape.mesh.indices[i + 2];

          auto prim = vec3i(i + 0, i + 1, i + 2);
          vi->push_back(prim);

          v->push_back(vec3f(attrib.vertices[idx0.vertex_index * 3 + 0],
                             attrib.vertices[idx0.vertex_index * 3 + 1],
                             attrib.vertices[idx0.vertex_index * 3 + 2]));
          v->push_back(vec3f(attrib.vertices[idx1.vertex_index * 3 + 0],
                             attrib.vertices[idx1.vertex_index * 3 + 1],
                             attrib.vertices[idx1.vertex_index * 3 + 2]));
          v->push_back(vec3f(attrib.vertices[idx2.vertex_index * 3 + 0],
                             attrib.vertices[idx2.vertex_index * 3 + 1],
                             attrib.vertices[idx2.vertex_index * 3 + 2]));

          // TODO create missing normals&texcoords if only some faces have them
          if (!attrib.normals.empty() && idx0.normal_index != -1) {
            vn->push_back(vec3f(attrib.normals[idx0.normal_index * 3 + 0],
                                attrib.normals[idx0.normal_index * 3 + 1],
                                attrib.normals[idx0.normal_index * 3 + 2]));
            vn->push_back(vec3f(attrib.normals[idx1.normal_index * 3 + 0],
                                attrib.normals[idx1.normal_index * 3 + 1],
                                attrib.normals[idx1.normal_index * 3 + 2]));
            vn->push_back(vec3f(attrib.normals[idx2.normal_index * 3 + 0],
                                attrib.normals[idx2.normal_index * 3 + 1],
                                attrib.normals[idx2.normal_index * 3 + 2]));
          }

          if (!attrib.texcoords.empty() && idx0.texcoord_index != -1) {
            vt->push_back(vec2f(attrib.texcoords[idx0.texcoord_index * 2 + 0],
                                attrib.texcoords[idx0.texcoord_index * 2 + 1]));
            vt->push_back(vec2f(attrib.texcoords[idx1.texcoord_index * 2 + 0],
                                attrib.texcoords[idx1.texcoord_index * 2 + 1]));
            vt->push_back(vec2f(attrib.texcoords[idx2.texcoord_index * 2 + 0],
                                attrib.texcoords[idx2.texcoord_index * 2 + 1]));
          }
        }

        mesh->add(v);
        mesh->add(vi);
        if (!vn->empty())
          mesh->add(vn);
        if (!vt->empty())
          mesh->add(vt);

        auto pmids = createNode("prim.materialID",
                                "DataVector1i")->nodeAs<DataVector1i>();

        auto numMatIds = shape.mesh.material_ids.size();
        pmids->v.reserve(numMatIds);

        for (auto id : shape.mesh.material_ids)
          pmids->v.push_back(id);

        mesh->add(pmids);
        mesh->add(sgMaterials);

        auto model = createNode(name + "_model", "Model");
        model->add(mesh);

        // TODO: Large .obj models with lots of groups run much slower with each
        //       group put in a separate instance. In the future, we want to
        //       support letting the user (ospExampleViewer, for starters)
        //       specify if each group should be placed in an instance or not.
#if USE_INSTANCES
        auto instance = createNode(name + "_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
#else
        (*objInstance)["model"].add(mesh);
#endif

      }

      std::cout << "...finished import!\n";
    }

    OSPSG_REGISTER_IMPORT_FUNCTION(importOBJ, obj);

  }  // ::ospray::sg
}  // ::ospray
