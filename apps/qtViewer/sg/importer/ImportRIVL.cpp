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

    void parseBGFscene(sg::World *world, xml::Node *root)
    {
      std::string rootName = root->name;
      if (rootName != "BGFscene")
        throw std::runtime_error("XML file is not a RIVL model !?");
      if (root->child.empty())
        throw std::runtime_error("emply RIVL model !?");

      Ref<sg::Node> lastNode;
      for (int childID=0;childID<root->child.size();childID++) {//xmlNode *node = root->children; node; node = node->next) {
        xml::Node *node = root->child[childID];
        std::string nodeName = node->name;
        if (nodeName == "text") {
          // -------------------------------------------------------
        } else if (nodeName == "Texture2D") {
          // -------------------------------------------------------
          Ref<sg::Texture2D> txt = new sg::Texture2D;
          txt.ptr->ospTexture = NULL;
          nodeList.push_back(txt.ptr);

          int height = -1, width = -1, ofs = -1, channels = -1, depth = -1;
          for (int pID=0;pID<node->prop.size();pID++) {
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
            }
          }
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
          //PRINT(txt.ptr->texel);
          // -------------------------------------------------------
        } else if (nodeName == "Material") {
          // -------------------------------------------------------
          Ref<sg::Material> mat = new sg::Material;
          mat->ospMaterial = NULL;
          nodeList.push_back(mat.ptr);

          // sg::Material *mat = RIVLmat.ptr->general.ptr;

          std::string name;
          std::string type;

          for (int pID=0;pID<node->prop.size();pID++) {
            xml::Prop *prop = node->prop[pID];
            // for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
            if (prop->name == "name") {
              // xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
              name = prop->value;//(const char*)value;
              mat->name = name;
              // mat->setParam("name", name.c_str());
              // mat->name = name;
              //xmlFree(value);
            } else if (prop->name == "type") {
              // xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
              type = prop->value; //(const char*)value;
              mat->type = type;
              // mat->setParam("type", type.c_str());
              //xmlFree(value);
            }
          }

          for (int childID=0;childID<node->child.size();childID++) {//xmlNode *child=node->children; child; child=child->next) {
            xml::Node *child = node->child[childID];
            std::string childNodeType = child->name;

            if (!childNodeType.compare("param")) {
              std::string childName;
              std::string childType;

              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr *attr = child->properties; attr; attr = attr->next) {
                if (prop->name == "name") {
                  // xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                  childName = prop->value; //(const char*)value;
                  //xmlFree(value);
                } else if (prop->name == "type") {
                  // xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
                  childType = prop->value; //(const char*)value;
                  //xmlFree(value);
                }
              }

              //Get the data out of the node
              // xmlChar *value = xmlNodeListGetString(node->doc, child->children, 1);
              char *value = strdup(child->content.c_str());
#define NEXT_TOK strtok(NULL, " \t\n\r")
              char *s = strtok((char*)value, " \t\n\r");
              //TODO: UGLY! Find a better way.
              if (!childType.compare("float")) {
                mat->setParam(childName,(float)atof(s));
                // mat->setParam(childName.c_str(), (float)atof(s));
              } else if (!childType.compare("float2")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                mat->setParam(childName,vec2f(x,y));
                // mat->setParam(childName.c_str(), vec2f(x,y));
              } else if (!childType.compare("float3")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                s = NEXT_TOK;
                float z = atof(s);
                mat->setParam(childName,vec3f(x,y,z));
              } else if (!childType.compare("float4")) {
                float x = atof(s);
                s = NEXT_TOK;
                float y = atof(s);
                s = NEXT_TOK;
                float z = atof(s);
                s = NEXT_TOK;
                float w = atof(s);
                mat->setParam(childName, vec4f(x,y,z,w));
              } else if (!childType.compare("int")) {
                mat->setParam(childName, (int32_t)atol(s));
              } else if (!childType.compare("int2")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                // mat->setParam(childName.c_str(), vec2i(x,y));
                mat->setParam(childName, vec2i(x,y));
              } else if (!childType.compare("int3")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                s = NEXT_TOK;
                int32_t z = atol(s);
                mat->setParam(childName, vec3i(x,y,z));
              } else if (!childType.compare("int4")) {
                int32_t x = atol(s);
                s = NEXT_TOK;
                int32_t y = atol(s);
                s = NEXT_TOK;
                int32_t z = atol(s);
                s = NEXT_TOK;
                int32_t w = atol(s);
                mat->setParam(childName, vec4i(x,y,z,w));
              } else {
                //error!
                throw std::runtime_error("unknown parameter type '" + childType + "' when parsing RIVL materials.");
              }
              free(value);
            } else if (!childNodeType.compare("textures")) {
              int num = -1;
              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr *attr = child->properties; attr; attr = attr->next) {
                if (prop->name == "num") {
                  num = atol(prop->value.c_str());
                  //xmlFree(value);
                }
              }

              // xmlChar *value = xmlNodaeListGetString(node->doc, child->children, 1);

              if (child->content == "") {
                // empty texture node ....
              } else {
                char *tokenBuffer = strdup(child->content.c_str());
                char *s = strtok(tokenBuffer, " \t\n\r");
                while (s) {
                  int texID = atoi(s);
                  Ref<Texture2D> tex = nodeList[texID].cast<Texture2D>().ptr;
                  tex->refInc();
                  mat->textures.push_back(tex);
                  s = NEXT_TOK;
                }
                free(tokenBuffer);
              }

              if (mat->textures.size() != num) {
                throw std::runtime_error("invalid number of textures in material "
                      "(found either more or less than the 'num' field specifies");
              }
            }
          }
#undef NEXT_TOK
          // -------------------------------------------------------
        } else if (nodeName == "Transform") {
          // -------------------------------------------------------
          Ref<sg::Transform> xfm = new sg::Transform;
          nodeList.push_back(xfm.ptr);

          // find child ID
          for (int pID=0;pID<node->prop.size();pID++) {
            xml::Prop *prop = node->prop[pID];
            // for (xmlAttr* attr = node->properties; attr; attr = attr->next)
            if (prop->name == "child") {
              // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
              size_t childID = atoi(prop->value.c_str());//(char*)value);
              sg::Node *child = (sg::Node *)nodeList[childID].ptr;
              assert(child);
              xfm->node = child;
              //xmlFree(value);
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
          //xmlFree(value);
          if (numRead != 12)  {
            throw std::runtime_error("invalid number of elements in RIVL transform node");
          }

          // -------------------------------------------------------
        } else if (nodeName == "Mesh") {
          // -------------------------------------------------------
          Ref<sg::PTMTriangleMesh> mesh = new sg::PTMTriangleMesh;
          nodeList.push_back(mesh.ptr);

          for (int childID=0;childID<node->child.size();childID++) {//xmlNode *child=node->children;child;child=child->next) {
            xml::Node *child = node->child[childID];
            std::string childType = child->name;
            if (childType == "text") {
            } else if (childType == "vertex") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (prop->name == "ofs") {
                  //xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
                else if (prop->name == "num") {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->vertex = make_aligned<DataArray3f>((char*)binBasePtr+ofs, num);
              // mesh->numVertices = num;
              // mesh->vertex = ;
            } else if (childType == "normal") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (prop->name == "ofs"){ //!strcmp((const char*)attr->name,"ofs")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
                else if (prop->name == "num") {//!strcmp((const char*)attr->name,"num")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->normal = new DataArray3f((vec3f*)((char*)binBasePtr+ofs),num,false);
              // mesh->numNormals = num;
              // mesh->normal = (vec3f*)(binBasePtr+ofs);
            } else if (childType == "texcoord") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (prop->name == "ofs") {//!strcmp((const char*)attr->name,"ofs")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  // ofs = atol((char*)value);
                  ofs = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
                else if (prop->name == "num") {//!strcmp((const char*)attr->name,"num")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  // num = atol((char*)value);
                  num = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              // mesh->numTexCoords = num;
              mesh->texcoord = new DataArray2f((vec2f*)((char*)binBasePtr+ofs),num,false);
              // mesh->texCoord = (vec2f*)(binBasePtr+ofs);
            } else if (childType == "prim") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (int pID=0;pID<child->prop.size();pID++) {
                xml::Prop *prop = child->prop[pID];
                // for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (prop->name == "ofs") {//!strcmp((const char*)attr->name,"ofs")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  // ofs = atol((char*)value);
                  ofs = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
                else if (prop->name == "num") {//!strcmp((const char*)attr->name,"num")) {
                  // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  // num = atol((char*)value);
                  num = atol(prop->value.c_str()); //(char*)value);
                  //xmlFree(value);
                }
                // if (prop->name == "ofs") {//!strcmp((const char*)attr->name,"ofs")) {
                //   xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                //   ofs = atol((char*)value);
                //   //xmlFree(value);
                // }
                // else if (prop->name == "num") {//!strcmp((const char*)attr->name,"num")) {
                //   xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                //   num = atol((char*)value);
                //   //xmlFree(value);
                // }
              }
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->index = make_aligned<DataArray4i>((char*)binBasePtr+ofs, num);
              // mesh->numTriangles = num;
              // mesh->triangle = (vec4i*)(binBasePtr+ofs);
            } else if (childType == "materiallist") {
              char* value = strdup(child->content.c_str()); //xmlNodeListGetString(node->doc, child->children, 1);
              // xmlChar* value = xmlNodeListGetString(node->doc, child->children, 1);
              for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
                size_t matID = atoi(s);
                Ref<sg::Material> mat = nodeList[matID].cast<sg::Material>();
                assert(mat.ptr);
                mat.ptr->refInc();
                mesh->materialList.push_back(mat);
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
          Ref<sg::Group> group = new sg::Group;
          nodeList.push_back(group.ptr);
          // xmlChar* value = xmlNodeListGetString(node->doc, node->children, 1);
          if (node->content == "")
            // empty group...
            ;
          // std::cout << "warning: xmlNodeListGetString(...) returned NULL" << std::endl;
          else {
            char *value = strdup(node->content.c_str());
            for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
              size_t childID = atoi(s);
              sg::Node *child = (sg::Node*)nodeList[childID].ptr;
              //assert(child);
              group->child.push_back(child);
            }
            free(value);
            //xmlFree(value);
          }
          lastNode = group.ptr;
        } else {
          nodeList.push_back(NULL);
          //throw std::runtime_error("unknown node type '"+nodeName+"' in RIVL model");
        }
      }
      //return lastNode;
      world->node.push_back(lastNode);
    }

    sg::World *importRIVL(const std::string &fileName)
    {
      string xmlFileName = fileName;
      string binFileName = fileName+".bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);

      xml::XMLDoc *doc = xml::readXML(fileName);
      if (doc->child.size() != 1 || doc->child[0]->name != "BGFscene")
        throw std::runtime_error("could not parse RIVL file: Not in RIVL format!?");
      xml::Node *root_element = doc->child[0];
      World *world = new World;
      parseBGFscene(world,root_element);
      delete doc;
      return world;
    }

  } // ::ospray::sg
} // ::ospray

