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

#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"
// stl
#include <map>
#include <iostream>

namespace ospray {
  namespace sg {
    using std::cout;
    using std::endl;
    using std::string;

    std::vector<std::shared_ptr<sg::Node>> nodeList;

    void *binBasePtr;

    void parseTextureNode(const xml::Node &node)
    {
      std::stringstream ss;
      ss << "rivlTexture_" << nodeList.size();
      const std::string name = ss.str();
      const std::string type = "Texture2D";
      std::shared_ptr<sg::Texture2D> txt = std::make_shared<sg::Texture2D>();
      txt->setName(name);
      txt->setType(type);
      txt->ospTexture = NULL;
      nodeList.push_back(txt);

      int height = -1, width = -1, ofs = -1, channels = -1, depth = -1;
      xml::for_each_prop(node,[&](const std::string &name, const std::string &value){
          if (name == "ofs")
            ofs = atol(value.c_str());
          else if (name == "width")
            width = atol(value.c_str());
          else if (name == "height")
            height = atol(value.c_str());
          else if (name == "channels")
            channels = atol(value.c_str());
          else if (name == "depth")
            depth = atol(value.c_str());
        });
      assert(ofs != -1 && "Offset not properly parsed for Texture2D nodes");
      assert(width != -1 && "Width not properly parsed for Texture2D nodes");
      assert(height != -1 && "Height not properly parsed for Texture2D nodes");
      assert(channels != -1 && "Channel count not properly parsed for Texture2D nodes");
      assert(depth != -1 && "Depth not properly parsed for Texture2D nodes");

      txt->texelType = OSP_TEXTURE_R8;
      if (channels == 4 && depth == 1)
        txt->texelType = OSP_TEXTURE_RGBA8;
      else if (channels == 3 && depth == 1)
        txt->texelType = OSP_TEXTURE_RGB8;
      else if (channels == 4)
        txt->texelType = OSP_TEXTURE_RGBA32F;
      else if (channels == 3)
        txt->texelType = OSP_TEXTURE_RGB32F;

      txt->size = vec2i(width, height);
      txt->channels = channels;
      txt->depth = depth;

      if (channels == 4) { // RIVL bin stores alpha channel inverted, fix here
        size_t sz = width * height;
        if (depth == 1) { // char
          vec4uc *texel = new vec4uc[sz];
          memcpy(texel, (char*)binBasePtr+ofs, sz*sizeof(vec4uc));
          for (size_t p = 0; p < sz; p++)
            texel[p].w = 255 - texel[p].w;
          txt->texelData = std::make_shared<DataArray1uc>((unsigned char*)texel,
                                                          width*height*sizeof(vec4uc),true);
        } else { // float
          vec4f *texel = new vec4f[sz];
          memcpy(texel, (char*)binBasePtr+ofs, sz*sizeof(vec4f));
          for (size_t p = 0; p < sz; p++)
            texel[p].w = 1.0f - texel[p].w;
          txt->texelData = std::make_shared<DataArray1uc>((unsigned char*)texel,
                                                          width*height*sizeof(vec4f),true);
        }
      } else
        txt->texelData = std::make_shared<DataArray1uc>((unsigned char*)(binBasePtr)+ofs,
                                                       width*height*3,false);
    }


    void parseMaterialTextures(std::shared_ptr<sg::Material> mat, const xml::Node &node)
    {
      size_t num = std::stoll(node.getProp("num"));
      if (node.content == "") {
        // empty texture node ....
      } else {
        char *tokenBuffer = strdup(node.content.c_str());
        char *s = strtok(tokenBuffer, " \t\n\r");
        while (s) {
          int texID = atoi(s);
          std::shared_ptr<Texture2D> tex = std::dynamic_pointer_cast<Texture2D>(nodeList[texID]);
          mat->textures.push_back(tex);
          s = strtok(NULL, " \t\n\r");
        }
        free(tokenBuffer);
      }

      if (mat->textures.size() != num) {
        throw std::runtime_error("invalid number of textures in material "
                                 "(found either more or less than the 'num' field specifies");
      }
    }

    void parseMaterialParam(std::shared_ptr<sg::Material> mat, const xml::Node &node)
    {
      const std::string paramName = node.getProp("name");
      const std::string paramType = node.getProp("type");

      //Get the data out of the node
      char *value = strdup(node.content.c_str());
#define NEXT_TOK strtok(NULL, " \t\n\r")
      char *s = strtok((char*)value, " \t\n\r");
      //TODO: UGLY! Find a better way.
      if (paramName.find("map_") != std::string::npos)
      {
        //TODO: lookup id into textures
        int texID = atoi(s);
          std::shared_ptr<Texture2D> tex = std::dynamic_pointer_cast<Texture2D>(mat->textures[texID]);
          s = strtok(NULL, " \t\n\r");
          mat->setChild(paramName, tex);
      }
      else if (!paramType.compare("float")) {
        mat->createChildWithValue(paramName,"float",(float)atof(s));
      } else if (!paramType.compare("float2")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        mat->createChildWithValue(paramName,"vec2f",vec2f(x,y));
      } else if (!paramType.compare("float3")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        s = NEXT_TOK;
        float z = atof(s);
        mat->createChildWithValue(paramName,"vec3f",vec3f(x,y,z));
      } else if (!paramType.compare("float4")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        s = NEXT_TOK;
        float z = atof(s);
        s = NEXT_TOK;
        float w = atof(s);
        mat->createChildWithValue(paramName, "vec4f",vec4f(x,y,z,w));
      } else if (!paramType.compare("int")) {
        mat->createChildWithValue(paramName, "int",(int32_t)atol(s));
      } else if (!paramType.compare("int2")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        mat->createChildWithValue(paramName, "vec2i",vec2i(x,y));
      } else if (!paramType.compare("int3")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        s = NEXT_TOK;
        int32_t z = atol(s);
        mat->createChildWithValue(paramName, "vec3i",vec3i(x,y,z));
      } else if (!paramType.compare("int4")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        s = NEXT_TOK;
        int32_t z = atol(s);
        s = NEXT_TOK;
        int32_t w = atol(s);
        mat->createChildWithValue(paramName, "vec4i",vec4i(x,y,z,w));
      } else {
        //error!
        throw std::runtime_error("unknown parameter type '" + paramType + "' when parsing RIVL materials.");
      }
      free(value);
    }

    void parseMaterialNode(const xml::Node &node)
    {
      std::shared_ptr<sg::Material> mat = std::make_shared<sg::Material>();
      nodeList.push_back(mat);

      mat->setName(node.getProp("name"));
      mat->child("type").setValue(node.getProp("type"));

      xml::for_each_child_of(node,[&](const xml::Node &child){
          if (!child.name.compare("textures"))
            parseMaterialTextures(mat,child);
          else if (!child.name.compare("param"))
            parseMaterialParam(mat,child);
        });
    }

    void parseTransformNode(const xml::Node &node)
    {
      std::shared_ptr<sg::Node> child;
      affine3f xfm;
      int id=0;
      size_t childID=0;

      // find child ID
      xml::for_each_prop(node,[&](const std::string &name, const std::string &value){
          if (name == "child") {
            childID = atoi(value.c_str());
            child = nodeList[childID];
            assert(child);
          } else if (name == "id") {
            id = atoi(value.c_str());
          }
        });

      // parse xfm matrix
      int numRead = sscanf((char*)node.content.c_str(),
                           "%f %f %f\n%f %f %f\n%f %f %f\n%f %f %f",
                           &xfm.l.vx.x,
                           &xfm.l.vx.y,
                           &xfm.l.vx.z,
                           &xfm.l.vy.x,
                           &xfm.l.vy.y,
                           &xfm.l.vy.z,
                           &xfm.l.vz.x,
                           &xfm.l.vz.y,
                           &xfm.l.vz.z,
                           &xfm.p.x,
                           &xfm.p.y,
                           &xfm.p.z);
      if (numRead != 12)  {
        throw std::runtime_error("invalid number of elements in RIVL transform node");
      }

      std::stringstream ss;
      ss << "transform_" << id;
      auto xfNode = createNode(ss.str(), "Transform");
      if (child->type() == "Model")
      {
        auto instance = createNode(ss.str(), "Instance");
        instance->setChild("model",child);
        child->setParent(instance);
        child = instance;
      }
      xfNode->setChild(child->name(), child);
      child->setParent(xfNode);
      std::static_pointer_cast<sg::Transform>(xfNode)->baseTransform = xfm;
      nodeList.push_back(std::dynamic_pointer_cast<sg::Node>(xfNode));
    }



    void parseMeshNode(const xml::Node &node)
    {
      std::shared_ptr<sg::TriangleMesh> mesh = std::make_shared<sg::TriangleMesh>();
      std::stringstream ss;
      ss << node.getProp("name");
      if (ss.str() == "")
        ss << "mesh";
      ss << "_" << node.getProp("id");
      mesh->setName(ss.str());
      mesh->setType("TriangleMesh");
      auto model = createNode ("model_"+ss.str(),"Model");
      model->add(mesh);
      nodeList.push_back(model);

      xml::for_each_child_of(node,[&](const xml::Node &child){
          assert(binBasePtr);
          if (child.name == "text") {
          } else if (child.name == "vertex") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            if (!binBasePtr)
              throw std::runtime_error("xml file mapping to binary file, but binary file not present");
            auto vertex =
              make_shared_aligned<DataArray3f>((char*)binBasePtr+ofs, num);
            vertex->setName("vertex");
            mesh->add(vertex);
          } else if (child.name == "normal") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            if (!binBasePtr)
              throw std::runtime_error("xml file mapping to binary file, but binary file not present");
            auto normal =
              std::make_shared<DataArray3f>((vec3f*)((char*)binBasePtr+ofs),
                                            num, false);
            normal->setName("normal");
            mesh->add(normal);
          } else if (child.name == "texcoord") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            if (!binBasePtr)
              throw std::runtime_error("xml file mapping to binary file, but binary file not present");
            auto texcoord =
              std::make_shared<DataArray2f>((vec2f*)((char*)binBasePtr+ofs),
                                            num, false);
            texcoord->setName("texcoord");
            mesh->add(texcoord);
          } else if (child.name == "prim") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            if (!binBasePtr)
              throw std::runtime_error("xml file mapping to binary file, but binary file not present");
            auto index =
              make_shared_aligned<DataArray4i>((char*)binBasePtr+ofs, num);
            index->setName("index");
            mesh->add(index);

            std::vector<uint32_t> matIDs;
            for( size_t i=0;i<index->size();i++)
              matIDs.push_back((*index)[i].w >> 16);
            auto ospPrimIDList = ospNewData(matIDs.size(), OSP_INT, matIDs.data());
            ospCommit(ospPrimIDList);
            mesh->ospPrimIDList = ospPrimIDList;
          } else if (child.name == "materiallist") {
            auto &materialList = mesh->materialList;
            char* value = strdup(child.content.c_str());
            int matCounter=0;
            auto &materialListNode = mesh->createChild("materialList", "Node");
            for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
              size_t matID = atoi(s);
              auto mat = nodeList[matID]->nodeAs<sg::Material>();
              mesh->setChild("material", mat);
              mat->setParent(mesh);
              materialList.push_back(mat);
              materialListNode.add(mat);
            }
            free(value);
          } else {
            throw std::runtime_error("unknown child node type '"+child.name+"' for mesh node");
          }
        });
    }

    void parseGroupNode(const xml::Node &node)
    {
      auto group = std::make_shared<sg::Group>();
      std::stringstream ss;
      ss << "group_" << nodeList.size();
      group->setName(ss.str());
      group->setType("Node");
      nodeList.push_back(group);
      if (!node.content.empty()) {
        char *value = strdup(node.content.c_str());

        for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
          size_t childID = atoi(s);
          auto child = nodeList[childID];

          if(!child)
            continue;

          group->children.push_back(child);
          std::stringstream ss;
          ss << "child_" << childID;
          if (child->type() == "Model") {
            auto instance = createNode(ss.str(), "Instance");
            instance->setChild("model",child);
            child->setParent(instance);
            child = instance;
          }
          group->setChild(ss.str(), child);
          child->setParent(group);
        }

        free(value);
      }
    }

    void parseBGFscene(std::shared_ptr<sg::Node> world, const xml::Node &root)
    {
      if (root.name != "BGFscene")
        throw std::runtime_error("XML file is not a RIVL model !?");
      if (root.child.empty())
        throw std::runtime_error("emply RIVL model !?");

      std::shared_ptr<sg::Node> lastNode;
      xml::for_each_child_of(root,[&](const xml::Node &node){
          if (node.name == "text") {
            // -------------------------------------------------------
          } else if (node.name == "Texture2D") {
            // -------------------------------------------------------
            parseTextureNode(node);
            // -------------------------------------------------------
          } else if (node.name == "Material") {
            // -------------------------------------------------------
            parseMaterialNode(node);
            // -------------------------------------------------------
          } else if (node.name == "Transform") {
            // -------------------------------------------------------
            parseTransformNode(node);
            lastNode = nodeList.back();
            // -------------------------------------------------------
          } else if (node.name == "Mesh") {
            // -------------------------------------------------------
            parseMeshNode(node);
            lastNode = nodeList.back();
            // -------------------------------------------------------
          } else if (node.name == "Group") {
            // -------------------------------------------------------
            parseGroupNode(node);
            lastNode = nodeList.back();
          } else {
            nodeList.push_back({});
          }
        });
      world->add(lastNode);
    }

    void importRIVL(std::shared_ptr<sg::Node> world,
                    const std::string &fileName)
    {
      string xmlFileName = fileName;
      string binFileName = fileName+".bin";
      binBasePtr = (void *)mapFile(binFileName);
      if (binBasePtr == nullptr) {
        std::cerr << "#osp:sg: WARNING: mapped file is NULL!!!!" << std::endl;
        std::cerr << "#osp:sg: WARNING: mapped file is NULL!!!!" << std::endl;
        std::cerr << "#osp:sg: WARNING: mapped file is NULL!!!!" << std::endl;
        std::cerr << "#osp:sg: WARNING: mapped file is NULL!!!!" << std::endl;
        std::cerr << "#osp:sg: WARNING: mapped file is NULL!!!!" << std::endl;
      }
      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fileName);
      if (doc->child.size() != 1 || doc->child[0]->name != "BGFscene")
        throw std::runtime_error("could not parse RIVL file: Not in RIVL format!?");
      const xml::Node &root_element = *doc->child[0];
      parseBGFscene(world,root_element);
    }

  } // ::ospray::sg
} // ::ospray

