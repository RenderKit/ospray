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

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

#define WARN_ON_INCLUDING_OSPCOMMON 1

// header
#include "miniSG.h"
// stl
#include <map>
#include <sstream>
// libxml
#include "common/xml/XML.h"
// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#endif
#include <fcntl.h>
#include <string>
#include <cstring>

namespace ospray {
  namespace miniSG {

    using std::string;
    using std::cout;
    using std::endl;

    //! base pointer to mmapped binary file (that all offsets are relative to)
    unsigned char *binBasePtr = nullptr;

    /*! Base class for all scene graph node types */
    struct Node : public ospcommon::RefCount
    {
      virtual string toString() const { return "ospray::miniSG::Node"; } 

      /*! \brief create a new instance of given type, and parse its
        content from given xml doc 

        doc may be nullptr, in which case a new instnace of given node
        type will be created with given default values for said
        type.

        if the given node type is not known or registered, this
        function may return nullptr.
      */

      std::string name;
    };

    /*! Abstraction for a 'material' that a renderer can query from
      any geometry (it's up to the renderer to know what to do with
      the respective mateiral type) */
    struct RIVLMaterial : public miniSG::Node {
      virtual string toString() const { return "ospray::miniSG::RIVLMaterial"; } 
      Ref<miniSG::Material> general;
    };

    struct RIVLCamera : public miniSG::Node {
      virtual string toString() const { return "ospray::miniSG::RIVLCamera"; } 
      vec3f from, at, up;
    };

    /*! Scene graph grouping node */
    struct Group : public miniSG::Node {
      virtual string toString() const;
      std::vector<Ref<miniSG::Node> > child;
    };

    /*! scene graph node that contains an ospray geometry type (i.e.,
      anything that defines some sort of geometric surface */
    struct Geometry : public miniSG::Node {
      std::vector<Ref<RIVLMaterial> > material;
      virtual string toString() const { return "ospray::miniSG::Geometry"; } 
    };

    /*! scene graph node that contains an ospray geometry type (i.e.,
      anything that defines some sort of geometric surface */
    struct RIVLTexture : public miniSG::Node {
      virtual string toString() const { return "ospray::miniSG::Texture"; } 
      Ref<miniSG::Texture2D> texData;
    };

    /*! scene graph node that contains an ospray geometry type (i.e.,
      anything that defines some sort of geometric surface */
    struct Transform : public miniSG::Node {
      Ref<miniSG::Node> child;
      affine3f xfm;
      virtual string toString() const { return "ospray::miniSG::Transform"; } 
    };

    /*! a triangle mesh with 3 floats and 3 ints for vertex and index
      data, respectively */
    // template<typename idx_t=ospray::vec3i,
    //          typename vtx_t=ospray::vec3f,
    //          typename nor_t=ospray::vec3f,
    //          typename txt_t=ospray::vec2f>
    struct TriangleMesh : public miniSG::Geometry {
      virtual string toString() const;
      TriangleMesh();

      // /*! \brief data handle to vertex data array. 
        
      //   can be null if user set the 'vertex' pointer manually */
      // Ref<ospray::Data> vertexData;
      // /*! \brief data handle to normal data array. 

      //   can be null if user set the 'normal' pointer manually */
      // Ref<ospray::Data> normalData;
      // /*! \brief data handle to txcoord data array. 

      //   can be null if user set the 'txcoord' pointer manually */
      // Ref<ospray::Data> texCoordData;
      // /*! \brief data handle to index data array. 

      //   can be null if user set the 'index' pointer manually */
      // Ref<ospray::Data> indexData;

      vec4i *triangle;
      size_t numTriangles;
      vec3f *vertex;
      size_t numVertices;
      vec3f *normal;
      size_t numNormals;
      vec2f *texCoord;
      size_t numTexCoords;
    };

    std::vector<Ref<miniSG::Node> > nodeList;

    TriangleMesh::TriangleMesh()
      : triangle(nullptr),
        numTriangles(0),
        vertex(nullptr),
        numVertices(0),
        normal(nullptr),
        numNormals(0),
        texCoord(nullptr),
        numTexCoords(0)
    {}

    std::string TriangleMesh::toString() const 
    { 
      std::stringstream ss;
      ss << "ospray::miniSG::TriangleMesh (";
      ss << numTriangles << " tris";
      if (numVertices)
        ss << ", " << numVertices << " vertices";
      if (numNormals)
        ss << ", " << numNormals << " normals";
      if (numTexCoords)
        ss << ", " << numTexCoords << " texCoords";
      if (material.size()) 
        ss << ", " << material.size() << " materials";
      ss << ")"; 
      return ss.str();
    } 
    

    std::string Group::toString() const 
    { 
      std::stringstream ss;
      ss << "ospray::miniSG::Group (" << child.size() << " children)"; 
      return ss.str();
    } 
    
    Ref<miniSG::Node> parseBGFscene(xml::Node *root)
    {
      std::string rootName = root->name;
      if (rootName != "BGFscene")
        throw std::runtime_error("XML file is not a RIVL model !?");
      if (root->child.empty())
        throw std::runtime_error("emply RIVL model !?");

      Ref<miniSG::Node> lastNode;
      for (size_t childID = 0; childID < root->child.size(); childID++) {
        xml::Node *node = root->child[childID];
        std::string nodeName = node->name;
        if (nodeName == "text") {
          // -------------------------------------------------------
        } else if (nodeName == "Texture2D") {
          // -------------------------------------------------------
          Ref<miniSG::RIVLTexture> txt = new miniSG::RIVLTexture;
          txt.ptr->texData = new miniSG::Texture2D;
          nodeList.push_back(txt.ptr);

          int height = -1, width = -1, ofs = -1, channels = -1, depth = -1;
          std::string format;

          for (size_t pID = 0; pID < node->prop.size(); pID++) {
            xml::Prop *prop = node->prop[pID];
            if (prop->name == "ofs") {
              ofs = atol(prop->value.c_str());
            } else if (prop->name == "width") {
              width = atol(prop->value.c_str());
            } else if (prop->name == "height") {
              height = atol(prop->value.c_str());
            } else if (prop->name == "channels") {
              channels = atol(prop->value.c_str());
            } else if (prop->name == "depth") {
              depth = atol(prop->value.c_str());
            } else if (prop->name == "format") {
              format = prop->value.c_str();
            }
          }
          assert(ofs != -1
                 && "Offset not properly parsed for Texture2D nodes");
          assert(width != -1
                 && "Width not properly parsed for Texture2D nodes");
          assert(height != -1
                 && "Height not properly parsed for Texture2D nodes");
          assert(channels != -1
                 && "Channel count not properly parsed for Texture2D nodes");
          assert(depth != -1
                 && "Depth not properly parsed for Texture2D nodes");
          assert(strcmp(format.c_str(), "") != 0
                 && "Format not properly parsed for Texture2D nodes");

          txt.ptr->texData->channels = channels;
          txt.ptr->texData->depth = depth;
          txt.ptr->texData->prefereLinear = true;
          txt.ptr->texData->width = width;
          txt.ptr->texData->height = height;
          if (channels == 4) { // RIVL bin stores alpha channel inverted, fix here
            size_t sz = width * height;
            if (depth == 1) { // char
              vec4uc *texel = new vec4uc[sz];
              memcpy(texel, binBasePtr+ofs, sz*sizeof(vec4uc));
              for (size_t p = 0; p < sz; p++)
                texel[p].w = 255 - texel[p].w; 
              txt.ptr->texData->data = texel;
            } else { // float
              vec4f *texel = new vec4f[sz];
              memcpy(texel, binBasePtr+ofs, sz*sizeof(vec4f));
              for (size_t p = 0; p < sz; p++)
                texel[p].w = 1.0f - texel[p].w; 
              txt.ptr->texData->data = texel;
            }
          } else
            txt.ptr->texData->data = (char*)(binBasePtr+ofs);

          // -------------------------------------------------------
        } else if (nodeName == "Material") {
          // -------------------------------------------------------
          Ref<miniSG::RIVLMaterial> RIVLmat = new miniSG::RIVLMaterial;
          RIVLmat.ptr->general = new miniSG::Material;
          nodeList.push_back(RIVLmat.ptr);

          miniSG::Material *mat = RIVLmat.ptr->general.ptr;

          std::string name;
          std::string type;

          for (size_t pID = 0; pID < node->prop.size(); pID++) {
            xml::Prop *prop = node->prop[pID];
            if (prop->name == "name") {
              name = prop->value;
              mat->setParam("name", name.c_str());
              mat->name = name;
            } else if (prop->name == "type") {
              type = prop->value;
              mat->setParam("type", type.c_str());
            }
          }

          for (size_t childID = 0; childID < node->child.size(); childID++) {
            xml::Node *child = node->child[childID];
            std::string childNodeType = child->name;

            if (!childNodeType.compare("param")) {
              std::string childName;
              std::string childType;

              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "name") {
                  childName = prop->value;
                } else if (prop->name == "type") { 
                  childType = prop->value;
                }
              }

              //Get the data out of the node
              char *value = strdup(child->content.c_str());
#define NEXT_TOK strtok(nullptr, " \t\n\r")
              char *s = strtok((char*)value, " \t\n\r");
              //TODO: UGLY! Find a better way.
              if (!childType.compare("float")) {
                mat->setParam(childName.c_str(), (float)atof(s));
              } else if (!childType.compare("float2")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                mat->setParam(childName.c_str(), vec2f(x,y));
              } else if (!childType.compare("float3")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                s = NEXT_TOK;
                float z = atof(s);
                mat->setParam(childName.c_str(), vec3f(x,y,z));
              } else if (!childType.compare("float4")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                s = NEXT_TOK;
                float z = atof(s);
                s = NEXT_TOK;
                float w = atof(s);
                mat->setParam(childName.c_str(), vec4f(x,y,z,w));
              } else if (!childType.compare("int")) {
                //This *could* be a texture, handle it!
                if(childName.find("map_") == std::string::npos) {
                  mat->setParam(childName.c_str(), (int32_t)atol(s));
                } else {
                  Texture2D* tex = mat->textures[(int32_t)atol(s)].ptr;
                  mat->setParam(childName.c_str(), (void*)tex, Material::Param::TEXTURE);
                }
              } else if (!childType.compare("int2")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                mat->setParam(childName.c_str(), vec2i(x,y));
              } else if (!childType.compare("int3")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                s = NEXT_TOK;
                int32_t z = atol(s);
                mat->setParam(childName.c_str(), vec3i(x,y,z));
              } else if (!childType.compare("int4")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                s = NEXT_TOK;
                int32_t z = atol(s);
                s = NEXT_TOK;
                int32_t w = atol(s);
                mat->setParam(childName.c_str(), vec4i(x,y,z,w));
              } else {
                //error!
                throw std::runtime_error("unknown parameter type '" + childType + "' when parsing RIVL materials.");
              }
              free(value);
            } else if (!childNodeType.compare("textures")) {
              int num = -1;
              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                }
              }

              if (child->content == "") {
              } else {
                char *tokenBuffer = strdup(child->content.c_str());
                
                char *s = strtok(tokenBuffer, " \t\n\r");
                while (s) {
                  int texID = atoi(s);
                  RIVLTexture * tex = nodeList[texID].cast<RIVLTexture>().ptr;
                  mat->textures.push_back(tex->texData);
                  s = NEXT_TOK;
                }
                free(tokenBuffer);
              }
              if (mat->textures.size() != static_cast<size_t>(num)) {
                throw std::runtime_error("invalid number of textures in"
                                         " material (found either more or less"
                                         " than the 'num' field specifies");
              }
            }
          }
#undef NEXT_TOK
          // -------------------------------------------------------
        } else if (nodeName == "Camera") {
          // -------------------------------------------------------
          Ref<miniSG::RIVLCamera> camera = new miniSG::RIVLCamera;
          nodeList.push_back(camera.ptr);

          // parse values
          for (size_t pID = 0; pID < node->child.size(); pID++) {
            xml::Node *childNode = node->child[pID];
            if (childNode->name == "from") {
              sscanf(childNode->content.c_str(),"%f %f %f",
                     &camera->from.x,&camera->from.y,&camera->from.z);
            }   
            if (childNode->name == "at") {
              sscanf(childNode->content.c_str(),"%f %f %f",
                     &camera->at.x,&camera->at.y,&camera->at.z);
            }   
            if (childNode->name == "up") {
              sscanf(childNode->content.c_str(),"%f %f %f",
                     &camera->up.x,&camera->up.y,&camera->up.z);
            }   
          }    
          // -------------------------------------------------------
        } else if (nodeName == "Transform") {
          // -------------------------------------------------------
          Ref<miniSG::Transform> xfm = new miniSG::Transform;
          nodeList.push_back(xfm.ptr);

          // find child ID
          for (size_t pID = 0;pID < node->prop.size(); pID++) {
            xml::Prop *prop = node->prop[pID];
            if (prop->name == "child") {
              size_t childID = atoi(prop->value.c_str());
              miniSG::Node *child = nodeList[childID].ptr;
              assert(child);
              xfm->child = child;
            }   
          }    
            
          // parse xfm matrix
          int numRead = sscanf((char*)node->content.c_str(),
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
          if (numRead != 12)  {
            throw std::runtime_error("invalid number of elements in RIVL"
                                     " transform node");
          }
          
          // -------------------------------------------------------
        } else if (nodeName == "Mesh") {
          // -------------------------------------------------------
          Ref<miniSG::TriangleMesh> mesh = new miniSG::TriangleMesh;
          nodeList.push_back(mesh.ptr);

          for (size_t childID = 0; childID < node->child.size(); childID++) {
            xml::Node *child = node->child[childID];
            std::string childType = child->name;
            if (childType == "text") {
            } else if (childType == "vertex") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "ofs") {
                  ofs = atol(prop->value.c_str());
                }       
                else if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                }       
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->numVertices = num;
              mesh->vertex = (vec3f*)(binBasePtr+ofs);
            } else if (childType == "normal") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "ofs"){
                  ofs = atol(prop->value.c_str());
                }       
                else if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                }       
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->numNormals = num;
              mesh->normal = (vec3f*)(binBasePtr+ofs);
            } else if (childType == "texcoord") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "ofs") {
                  ofs = atol(prop->value.c_str());
                }       
                else if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                }       
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->numTexCoords = num;
              mesh->texCoord = (vec2f*)(binBasePtr+ofs);
            } else if (childType == "prim") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (size_t pID = 0; pID < child->prop.size(); pID++) {
                xml::Prop *prop = child->prop[pID];
                if (prop->name == "ofs") {
                  ofs = atol(prop->value.c_str());
                }       
                else if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                }       
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->numTriangles = num;
              mesh->triangle = (vec4i*)(binBasePtr+ofs);
            } else if (childType == "materiallist") {
              char* value = strdup(child->content.c_str());
              for(char *s=strtok((char*)value," \t\n\r");
                  s;
                  s=strtok(nullptr," \t\n\r")) {
                size_t matID = atoi(s);
                Ref<RIVLMaterial> mat = nodeList[matID].cast<miniSG::RIVLMaterial>();
                mat.ptr->refInc();
                assert(mat.ptr);
                mesh->material.push_back(mat);
              }
              free(value);
              //xmlFree(value);
            } else {
              throw std::runtime_error("unknown child node type '"+childType+"' for mesh node");
            }
          }
          // std::cout << "Found mesh " << mesh->toString() << std::endl;
          lastNode = mesh.ptr;
          // -------------------------------------------------------
        } else if (nodeName == "Group") {
          // -------------------------------------------------------
          Ref<miniSG::Group> group = new miniSG::Group;
          nodeList.push_back(group.ptr);
          // xmlChar* value = xmlNodeListGetString(node->doc, node->children, 1);
          if (node->content == "")
            // empty group...
            ;
          // std::cout << "warning: xmlNodeListGetString(...) returned nullptr" << std::endl;
          else {
            char *value = strdup(node->content.c_str());
            for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(nullptr," \t\n\r")) {
              size_t childID = atoi(s);
              miniSG::Node *child = nodeList[childID].ptr;
              //assert(child);
              group->child.push_back(child);
            }
            free(value);
            //xmlFree(value);
          }
          lastNode = group.ptr;
        } else {
          nodeList.push_back(nullptr);
          //throw std::runtime_error("unknown node type '"+nodeName+"' in RIVL model");
        }
      }
      return lastNode;
    }

    Ref<miniSG::Node> importRIVL(const std::string &fileName)
    {
      string xmlFileName = fileName;
      string binFileName = fileName+".bin";

      FILE *file = fopen(binFileName.c_str(),"rb");
      if (!file)
        perror("could not open binary file");
      fseek(file,0,SEEK_END);
      ssize_t fileSize =
#ifdef _WIN32
        _ftelli64(file);
#else
        ftell(file);
#endif
      fclose(file);
      
#ifdef _WIN32
      HANDLE fileHandle = CreateFile(binFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
      if (fileHandle == nullptr)
        fprintf(stderr, "could not open file '%s' (error %lu)\n", binFileName.c_str(), GetLastError());
      HANDLE fileMappingHandle = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
      if (fileMappingHandle == nullptr)
        fprintf(stderr, "could not create file mapping (error %lu)\n", GetLastError());
#else
      int fd = ::open(binFileName.c_str(), O_LARGEFILE | O_RDONLY);
      if (fd == -1)
        perror("could not open file");
#endif

      binBasePtr = (unsigned char *)
#ifdef _WIN32
        MapViewOfFile(fileMappingHandle, FILE_MAP_READ, 0, 0, fileSize);
#else
        mmap(nullptr,fileSize,PROT_READ,MAP_SHARED,fd,0);
#endif

      xml::XMLDoc *doc = xml::readXML(fileName);
      if (doc->child.size() != 1 || doc->child[0]->name != "BGFscene") 
        throw std::runtime_error("could not parse RIVL file: Not in RIVL format!?");
      xml::Node *root_element = doc->child[0];
      Ref<Node> node = parseBGFscene(root_element);
      return node;
    }

    void traverseSG(Model &model, Ref<miniSG::Node> &node, const affine3f &xfm=ospcommon::one)
    {
      Group *g = dynamic_cast<Group *>(node.ptr);
      if (g) {
        for (size_t i = 0; i < g->child.size(); i++)
          traverseSG(model,g->child[i],xfm);
        return;
      }

      Transform *xf = dynamic_cast<Transform *>(node.ptr);
      if (xf) {
        traverseSG(model,xf->child,xfm*xf->xfm);
        return;
      }

      RIVLCamera *cam = dynamic_cast<RIVLCamera *>(node.ptr);
      if (cam) {
        Ref<miniSG::Camera> c = new miniSG::Camera;
        c->from = cam->from;
        c->up = cam->up;
        c->at = cam->at;
        model.camera.push_back(c);
        return;
      }

      TriangleMesh *tm = dynamic_cast<TriangleMesh *>(node.ptr);
      if (tm) {
        static std::map<TriangleMesh *, int> meshIDs;
        if (meshIDs.find(tm) == meshIDs.end()) {
          meshIDs[tm] = model.mesh.size();
          Mesh *mesh = new Mesh;
          mesh->position.resize(tm->numVertices);
          mesh->triangle.resize(tm->numTriangles);
          mesh->triangleMaterialId.resize(tm->numTriangles);
          bool anyNotZero = false;
          for (size_t i = 0; i < tm->numTriangles; i++) {
            Triangle t;
            t.v0 = tm->triangle[i].x;
            t.v1 = tm->triangle[i].y;
            t.v2 = tm->triangle[i].z;

            mesh->triangle[i] = t;

            assert(mesh->triangle[i].v0 >= 0
                   && mesh->triangle[i].v0 < mesh->position.size());
            assert(mesh->triangle[i].v1 >= 0
                   && mesh->triangle[i].v1 < mesh->position.size());
            assert(mesh->triangle[i].v2 >= 0
                   && mesh->triangle[i].v2 < mesh->position.size());

            mesh->triangleMaterialId[i] = tm->triangle[i].w >>16;
            if (mesh->triangleMaterialId[i]) anyNotZero = true;
          }
          if (!anyNotZero)
            mesh->triangleMaterialId.clear();

          for (size_t i = 0; i < tm->numVertices; i++) {
            mesh->position[i].x = tm->vertex[i].x;
            mesh->position[i].y = tm->vertex[i].y;
            mesh->position[i].z = tm->vertex[i].z;
            mesh->position[i].w = 0;
          }
          if (tm->numNormals > 0) {
            mesh->normal.resize(tm->numVertices);
            for (size_t i = 0; i < tm->numNormals; i++) {
              mesh->normal[i].x = tm->normal[i].x;
              mesh->normal[i].y = tm->normal[i].y;
              mesh->normal[i].z = tm->normal[i].z;
              mesh->normal[i].w = 0;
            }
          }
          if (tm->numTexCoords > 0) {
            mesh->texcoord.resize(tm->numVertices);
            for (size_t i = 0; i < tm->numTexCoords; i++) {
              (vec2f&)mesh->texcoord[i] = (vec2f&)tm->texCoord[i];
            }
          }
          model.mesh.push_back(mesh);

          if (tm->material.size() == 1) {
            mesh->material = tm->material[0].ptr->general;
          } else {
            for (size_t i = 0; i < tm->material.size(); i++) {
              mesh->materialList.push_back(tm->material[i].ptr->general);
            }
          }
        }

        int meshID = meshIDs[tm];
        model.instance.push_back(Instance(meshID,xfm));
        
        return;
      }

      RIVLMaterial *mt = dynamic_cast<RIVLMaterial *>(node.ptr);
      if (mt) {
        return;
      }

      throw std::runtime_error("unhandled node type '"
                               + node->toString() + "' in traverseSG");
    }

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importRIVL(Model &model, const ospcommon::FileName &fileName)
    {
      nodeList.clear();
      Ref<miniSG::Node> sg = importRIVL(fileName);
      traverseSG(model,sg);
      nodeList.clear();
      sg = 0;
    }
    
  } // ::ospray::minisg
} // ::ospray
