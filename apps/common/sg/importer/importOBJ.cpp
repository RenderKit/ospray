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
#include <sstream>

namespace ospray {
  namespace sg {

    std::shared_ptr<Texture2D> loadTexture(const FileName &fullPath,
                                           const bool preferLinear = false)
    {
      std::shared_ptr<Texture2D> tex = Texture2D::load(fullPath,
                                                       preferLinear);
      if (!tex)
        std::cout << "could not load texture " << fullPath.str() << " !\n";

      return tex;
    }

    void addTextureIfNeeded(Material &node,
                            const std::string &type,
                            const FileName &texName,
                            const FileName &containingPath,
                            bool preferLinear = false)
    {
      if (!texName.str().empty()) {
        auto tex = loadTexture(containingPath + texName, preferLinear);
        if (tex) {
          tex->setName(type);
          node.setChild(type, tex);
        }
      }
    }
    
    static inline float parseFloatString(std::string valueString)
    {
      std::stringstream valueStream(valueString);
      
      float value;
      valueStream >> value;
      
      return value;
    }
    
    static inline vec3f parseVec3fString(std::string valueString)
    {
      std::stringstream valueStream(valueString);
      
      vec3f value;
      valueStream >> value.x >> value.y >> value.z;
      
      return value;
    }
    
    static inline void parseParameterString(std::string typeAndValueString,
                                            std::string & paramType,
                                            ospcommon::utility::Any & paramValue)
    {
      std::stringstream typeAndValueStream(typeAndValueString);
      
      typeAndValueStream >> paramType;
      
      std::string paramValueString;
      getline(typeAndValueStream,paramValueString);
      
      if (paramType == "float") {
        paramValue = parseFloatString(paramValueString);
      } else if (paramType == "vec3f") {
        paramValue = parseVec3fString(paramValueString);
      } else {
        // Unknown type.
        paramValue = typeAndValueString;
      }
    }

    static inline std::vector<std::shared_ptr<Material>>
    createSgMaterials(std::vector<tinyobj::material_t> &mats,
                      const FileName &containingPath)
    {
      std::vector<std::shared_ptr<Material>> sgMaterials;

      for (auto &mat : mats) {
        auto matNodePtr = createNode(mat.name, "Material")->nodeAs<Material>();
        auto &matNode = *matNodePtr;

        for (auto &param : mat.unknown_parameter) {
          if (param.first == "type") {
            matNode["type"].setValue(param.second);
            std::cout << "Creating material node of type " << param.second << std::endl;
          } else {
            std::string paramType;
            ospcommon::utility::Any paramValue;
            parseParameterString(param.second, paramType, paramValue);
            matNode.createChildWithValue(param.first, paramType, paramValue);
            std::cout << "Parsed parameter " << param.first << " of type " << paramType << std::endl;
          }
        }

        matNode["d"].setValue(mat.dissolve);
        matNode["Ka"].setValue(vec3f(mat.ambient[0],
                                     mat.ambient[1],
                                     mat.ambient[2]));
        matNode["Kd"].setValue(vec3f(mat.diffuse[0],
                                     mat.diffuse[1],
                                     mat.diffuse[2]));
        matNode["Ks"].setValue(vec3f(mat.specular[0],
                                     mat.specular[1],
                                     mat.specular[2]));

        addTextureIfNeeded(matNode, "map_Ka", mat.ambient_texname,
                           containingPath);
        addTextureIfNeeded(matNode, "map_Kd", mat.diffuse_texname,
                           containingPath);
        addTextureIfNeeded(matNode, "map_Ks", mat.specular_texname,
                           containingPath);
        addTextureIfNeeded(matNode, "map_Ns", mat.specular_highlight_texname,
                           containingPath, true);
        addTextureIfNeeded(matNode, "map_bump", mat.bump_texname,
                           containingPath);
        addTextureIfNeeded(matNode, "map_d", mat.alpha_texname,
                           containingPath, true);

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

      auto sgMaterials = createSgMaterials(materials, containingPath);

      std::string base_name = fileName.name() + '_';
      int shapeId = 0;

      for (auto &shape : shapes) {
        for (int numVertsInFace : shape.mesh.num_face_vertices) {
          if (numVertsInFace != 3) {
            std::cerr << "Warning: more thant 3 verts in face!";
            PRINT(numVertsInFace);
          }
        }

        auto name = base_name + std::to_string(shapeId++) + '_' + shape.name;
        auto mesh = createNode(name, "TriangleMesh")->nodeAs<TriangleMesh>();

        auto v = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        auto numSrcIndices = shape.mesh.indices.size();
        v->v.reserve(numSrcIndices);

        auto vi = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
        vi->v.reserve(numSrcIndices / 3);

        auto vn = createNode("normal", "DataVector3f")->nodeAs<DataVector3f>();
        vn->v.reserve(numSrcIndices);

        auto vt = createNode("texcoord","DataVector2f")->nodeAs<DataVector2f>();
        vt->v.reserve(numSrcIndices);

        for (int i = 0; i < shape.mesh.indices.size(); i += 3) {
          auto idx0 = shape.mesh.indices[i+0];
          auto idx1 = shape.mesh.indices[i+1];
          auto idx2 = shape.mesh.indices[i+2];

          auto prim = vec3i(i+0, i+1, i+2);
          vi->push_back(prim);

          v->push_back(vec3f(attrib.vertices[idx0.vertex_index*3+0],
                             attrib.vertices[idx0.vertex_index*3+1],
                             attrib.vertices[idx0.vertex_index*3+2]));
          v->push_back(vec3f(attrib.vertices[idx1.vertex_index*3+0],
                             attrib.vertices[idx1.vertex_index*3+1],
                             attrib.vertices[idx1.vertex_index*3+2]));
          v->push_back(vec3f(attrib.vertices[idx2.vertex_index*3+0],
                             attrib.vertices[idx2.vertex_index*3+1],
                             attrib.vertices[idx2.vertex_index*3+2]));

          if (!attrib.normals.empty()) {
            vn->push_back(vec3f(attrib.normals[idx0.normal_index*3+0],
                                attrib.normals[idx0.normal_index*3+1],
                                attrib.normals[idx0.normal_index*3+2]));
            vn->push_back(vec3f(attrib.normals[idx1.normal_index*3+0],
                                attrib.normals[idx1.normal_index*3+1],
                                attrib.normals[idx1.normal_index*3+2]));
            vn->push_back(vec3f(attrib.normals[idx2.normal_index*3+0],
                                attrib.normals[idx2.normal_index*3+1],
                                attrib.normals[idx2.normal_index*3+2]));
          }

          if (!attrib.texcoords.empty()) {
            vt->push_back(vec2f(attrib.texcoords[idx0.texcoord_index*2+0],
                                attrib.texcoords[idx0.texcoord_index*2+1]));
            vt->push_back(vec2f(attrib.texcoords[idx1.texcoord_index*2+0],
                                attrib.texcoords[idx1.texcoord_index*2+1]));
            vt->push_back(vec2f(attrib.texcoords[idx2.texcoord_index*2+0],
                                attrib.texcoords[idx2.texcoord_index*2+1]));
          }
        }

        mesh->add(v);
        mesh->add(vi);
        if(!vn->empty()) mesh->add(vn);
        if(!vt->empty()) mesh->add(vt);

        auto matIdx = shape.mesh.material_ids[0];
        if (!sgMaterials.empty()) {
          if (matIdx >= 0)
            mesh->setChild("material", sgMaterials[matIdx]);
          else
            mesh->setChild("material", sgMaterials[0]);
        }

        auto model = createNode(name + "_model", "Model");
        model->add(mesh);

        auto instance = createNode(name + "_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
      }
    }

  } // ::ospray::sg
} // ::ospray

