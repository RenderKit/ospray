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

#undef NDEBUG

#define WARN_ON_INCLUDING_OSPCOMMON 1

#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"
// stl
#include <map>

namespace ospray {
  namespace sg {
    using std::cout;
    using std::endl;
    using std::string;

    std::vector<Ref<sg::Node> > nodeList;

    void *binBasePtr;

    void parseTextureNode(const xml::Node &node)
    {
      Ref<sg::Texture2D> txt = new sg::Texture2D;
      txt.ptr->ospTexture = NULL;
      nodeList.push_back(txt.ptr);

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
      assert(ofs != size_t(-1) && "Offset not properly parsed for Texture2D nodes");
      assert(width != size_t(-1) && "Width not properly parsed for Texture2D nodes");
      assert(height != size_t(-1) && "Height not properly parsed for Texture2D nodes");
      assert(channels != size_t(-1) && "Channel count not properly parsed for Texture2D nodes");
      assert(depth != size_t(-1) && "Depth not properly parsed for Texture2D nodes");

      txt.ptr->texelType = OSP_TEXTURE_R8;
      if (channels == 4 && depth == 1)
        txt.ptr->texelType = OSP_TEXTURE_RGBA8;
      else if (channels == 3 && depth == 1)
        txt.ptr->texelType = OSP_TEXTURE_RGB8;
      else if (channels == 4)
        txt.ptr->texelType = OSP_TEXTURE_RGBA32F;
      else if (channels == 3)
        txt.ptr->texelType = OSP_TEXTURE_RGB32F;

      txt.ptr->size = vec2i(width, height);

      if (channels == 4) { // RIVL bin stores alpha channel inverted, fix here
        size_t sz = width * height;
        if (depth == 1) { // char
          vec4uc *texel = new vec4uc[sz];
          memcpy(texel, (char*)binBasePtr+ofs, sz*sizeof(vec4uc));
          for (size_t p = 0; p < sz; p++)
            texel[p].w = 255 - texel[p].w;
          txt.ptr->texel = texel;
        } else { // float
          vec4f *texel = new vec4f[sz];
          memcpy(texel, (char*)binBasePtr+ofs, sz*sizeof(vec4f));
          for (size_t p = 0; p < sz; p++)
            texel[p].w = 1.0f - texel[p].w;
          txt.ptr->texel = texel;
        }
      } else
        txt.ptr->texel = (char*)(binBasePtr)+ofs;
    }


    void parseMaterialTextures(Ref<sg::Material> mat, const xml::Node &node)
    {
      size_t num = std::stoll(node.getProp("num"));
      if (node.content == "") {
        // empty texture node ....
      } else {
        char *tokenBuffer = strdup(node.content.c_str());
        char *s = strtok(tokenBuffer, " \t\n\r");
        while (s) {
          int texID = atoi(s);
          Ref<Texture2D> tex = nodeList[texID].cast<Texture2D>().ptr;
          tex->refInc();
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

    void parseMaterialParam(Ref<sg::Material> mat, const xml::Node &node)
    {
      const std::string paramName = node.getProp("name");
      const std::string paramType = node.getProp("type");
      
      //Get the data out of the node
      char *value = strdup(node.content.c_str());
#define NEXT_TOK strtok(NULL, " \t\n\r")
      char *s = strtok((char*)value, " \t\n\r");
      //TODO: UGLY! Find a better way.
      if (!paramType.compare("float")) {
        mat->setParam(paramName,(float)atof(s));
      } else if (!paramType.compare("float2")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        mat->setParam(paramName,vec2f(x,y));
      } else if (!paramType.compare("float3")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        s = NEXT_TOK;
        float z = atof(s);
        mat->setParam(paramName,vec3f(x,y,z));
      } else if (!paramType.compare("float4")) {
        float x = atof(s);
        s = NEXT_TOK;
        float y = atof(s);
        s = NEXT_TOK;
        float z = atof(s);
        s = NEXT_TOK;
        float w = atof(s);
        mat->setParam(paramName, vec4f(x,y,z,w));
      } else if (!paramType.compare("int")) {
        mat->setParam(paramName, (int32_t)atol(s));
      } else if (!paramType.compare("int2")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        mat->setParam(paramName, vec2i(x,y));
      } else if (!paramType.compare("int3")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        s = NEXT_TOK;
        int32_t z = atol(s);
        mat->setParam(paramName, vec3i(x,y,z));
      } else if (!paramType.compare("int4")) {
        int32_t x = atol(s);
        s = NEXT_TOK;
        int32_t y = atol(s);
        s = NEXT_TOK;
        int32_t z = atol(s);
        s = NEXT_TOK;
        int32_t w = atol(s);
        mat->setParam(paramName, vec4i(x,y,z,w));
      } else {
        //error!
        throw std::runtime_error("unknown parameter type '" + paramType + "' when parsing RIVL materials.");
      }
      free(value);
    }
      
    void parseMaterialNode(const xml::Node &node)
    {
      Ref<sg::Material> mat = new sg::Material;
      mat->ospMaterial = NULL;
      nodeList.push_back(mat.ptr);
      
      mat->name = node.getProp("name","");
      mat->type = node.getProp("type","");
      
      xml::for_each_child_of(node,[&](const xml::Node &child){
          if (!child.name.compare("textures")) 
            parseMaterialTextures(mat,child);
          else if (!child.name.compare("param"))
            parseMaterialParam(mat,child);
        });
    }

    void parseTransformNode(const xml::Node &node)
    {
      Ref<sg::Transform> xfm = new sg::Transform;
      nodeList.push_back(xfm.ptr);

      // find child ID
      xml::for_each_prop(node,[&](const std::string &name, const std::string &value){
          if (name == "child") {
            size_t childID = atoi(value.c_str());//(char*)value);
            sg::Node *child = (sg::Node *)nodeList[childID].ptr;
            assert(child);
            xfm->node = child;
          }
        });

      // parse xfm matrix
      int numRead = sscanf((char*)node.content.c_str(),
                           "%f %f %f\n%f %f %f\n%f %f %f\n%f %f %f",
                           &xfm->xfm.l.vx.x,
                           &xfm->xfm.l.vx.y,
                           &xfm->xfm.l.vx.z,
                           &xfm->xfm.l.vy.x,
                           &xfm->xfm.l.vy.y,
                           &xfm->xfm.l.vy.z,
                           &xfm->xfm.l.vz.x,
                           &xfm->xfm.l.vz.y,
                           &xfm->xfm.l.vz.z,
                           &xfm->xfm.p.x,
                           &xfm->xfm.p.y,
                           &xfm->xfm.p.z);
      //xmlFree(value);
      if (numRead != 12)  {
        throw std::runtime_error("invalid number of elements in RIVL transform node");
      }
    }



    void parseMeshNode(const xml::Node &node)
    {
      Ref<sg::PTMTriangleMesh> mesh = new sg::PTMTriangleMesh;
      nodeList.push_back(mesh.ptr);

      xml::for_each_child_of(node,[&](const xml::Node &child){
          assert(binBasePtr);
          if (child.name == "text") {
          } else if (child.name == "vertex") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            mesh->vertex = make_aligned<DataArray3f>((char*)binBasePtr+ofs, num);
          } else if (child.name == "normal") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            mesh->normal = new DataArray3f((vec3f*)((char*)binBasePtr+ofs),num,false);
          } else if (child.name == "texcoord") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            mesh->texcoord = new DataArray2f((vec2f*)((char*)binBasePtr+ofs),num,false);
          } else if (child.name == "prim") {
            size_t ofs      = std::stoll(child.getProp("ofs"));
            size_t num      = std::stoll(child.getProp("num"));
            mesh->index = make_aligned<DataArray4i>((char*)binBasePtr+ofs, num);
          } else if (child.name == "materiallist") {
            char* value = strdup(child.content.c_str());
            for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
              size_t matID = atoi(s);
              Ref<sg::Material> mat = nodeList[matID].cast<sg::Material>();
              assert(mat.ptr);
              mat.ptr->refInc();
              mesh->materialList.push_back(mat);
            }
            free(value);
          } else {
            throw std::runtime_error("unknown child node type '"+child.name+"' for mesh node");
          }
        });
    }

    void parseGroupNode(const xml::Node &node)
    {
      Ref<sg::Group> group = new sg::Group;
      nodeList.push_back(group.ptr);
      if (node.content == "")
        // empty group...
        ;
      else {
        char *value = strdup(node.content.c_str());
        for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
          size_t childID = atoi(s);
          sg::Node *child = (sg::Node*)nodeList[childID].ptr;
          group->child.push_back(child);
        }
        free(value);
      }
    }
    
    void parseBGFscene(sg::World *world, const xml::Node &root)
    {
      if (root.name != "BGFscene")
        throw std::runtime_error("XML file is not a RIVL model !?");
      if (root.child.empty())
        throw std::runtime_error("emply RIVL model !?");
      
      Ref<sg::Node> lastNode;
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
            nodeList.push_back(NULL);
            //throw std::runtime_error("unknown node type '"+node.name+"' in RIVL model");
          }
        });
      PING;
      PRINT(lastNode);
      PRINT(lastNode->toString());
      world->node.push_back(lastNode);
    }

    sg::World *importRIVL(const std::string &fileName)
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
      World *world = new World;
      parseBGFscene(world,root_element);
      PRINT(world->getBounds());
      return world;
    }

  } // ::ospray::sg
} // ::ospray

