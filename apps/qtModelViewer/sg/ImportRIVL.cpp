/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// header
#include "Scene.h"
// stl
#include <map>
// // libxml
#include "apps/common/xml/xml.h"
// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace ospray {
  namespace viewer {
    namespace rivl {
      using std::string;
      using std::cout;
      using std::endl;

      //! base pointer to mmapped binary file (that all offsets are relative to)
      unsigned char *binBasePtr = NULL;

      /*! Base class for all scene graph node types */
      struct Node : public sg::Node
      {
        virtual string toString() const { return "ospray::rivl::Node"; } 

        /*! \brief create a new instance of given type, and parse its
          content from given xml doc 

          doc may be NULL, in which case a new instnace of given node
          type will be created with given default values for said
          type.

          if the given node type is not known or registered, this
          function may return NULL.
        */

        std::string name;
      };

      /*! Scene graph grouping node */
      struct Group : public rivl::Node {
        virtual string toString() const;
        std::vector<Ref<rivl::Node> > child;
      };

      /*! scene graph node that contains an ospray geometry type (i.e.,
        anything that defines some sort of geometric surface */
      struct Geometry : public rivl::Node {
        std::vector<Ref<sg::Material> > material;
        virtual string toString() const { return "ospray::rivl::Geometry"; } 
      };

      /*! scene graph node that contains an ospray geometry type (i.e.,
        anything that defines some sort of geometric surface */
      struct RIVLTexture : public rivl::Node {
        virtual string toString() const { return "ospray::rivl::Texture"; } 
        Ref<sg::Texture2D> texData;
      };

      /*! scene graph node that contains an ospray geometry type (i.e.,
        anything that defines some sort of geometric surface */
      struct Transform : public rivl::Node {
        Ref<rivl::Node> child;
        affine3f xfm;
        virtual string toString() const { return "ospray::rivl::Transform"; } 
      };

      /*! a triangle mesh with 3 floats and 3 ints for vertex and index
        data, respectively */
      // template<typename idx_t=ospray::vec3i,
      //          typename vtx_t=ospray::vec3f,
      //          typename nor_t=ospray::vec3f,
      //          typename txt_t=ospray::vec2f>
      struct TriangleMesh : public rivl::Geometry {
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

      std::vector<Ref<sg::Node> > nodeList;

      TriangleMesh::TriangleMesh()
        : triangle(NULL),
          numTriangles(0),
          vertex(NULL),
          numVertices(0),
          normal(NULL),
          numNormals(0),
          texCoord(NULL),
          numTexCoords(0)
      {}

      std::string TriangleMesh::toString() const 
      { 
        std::stringstream ss;
        ss << "ospray::rivl::TriangleMesh (";
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
        ss << "ospray::rivl::Group (" << child.size() << " children)"; 
        return ss.str();
      } 
    
      Ref<rivl::Node> parseBGFscene(xml::Node *root)
      {
        std::string rootName = root->name;
        if (rootName != "BGFscene")
          throw std::runtime_error("XML file is not a RIVL model !?");
        if (root->child.empty())
          throw std::runtime_error("emply RIVL model !?");

        Ref<rivl::Node> lastNode;
        for (int childID=0;childID<root->child.size();childID++) {//xmlNode *node = root->children; node; node = node->next) {
          xml::Node *node = root->child[childID];
          std::string nodeName = node->name;
          if (nodeName == "text") {
            // -------------------------------------------------------
          } else if (nodeName == "Texture2D") {
            // -------------------------------------------------------
            Ref<rivl::RIVLTexture> txt = new rivl::RIVLTexture;
            txt.ptr->texData = new sg::Texture2D;
            nodeList.push_back(txt.ptr);

            int height = -1, width = -1, ofs = -1, channels = -1, depth = -1;
            std::string format;

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
              } else if (prop->name == "format") {
                format = prop->value.c_str();
              }
            }
            // for (xmlAttr *attr = node->properties; attr; attr = attr->next) {
            //   if (!strcmp( (const char*)attr->name, "ofs" )) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     ofs = atol((char*)value);
            //     //xmlFree(value);
            //   } else if (!strcmp((const char*)attr->name, "width")) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     width = atol((char*)value);
            //     //xmlFree(value);
            //   } else if (!strcmp((const char*)attr->name, "height")) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     height = atol((char*)value);
            //     //xmlFree(value);
            //   } else if (!strcmp((const char*)attr->name, "channels")) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     channels = atol((char*)value);
            //     //xmlFree(value);
            //   } else if (!strcmp((const char*)attr->name, "depth")) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     depth = atol((char*)value);
            //     //xmlFree(value);
            //   } else if (!strcmp((const char*)attr->name, "format")) {
            //     xmlChar *value = xmlNodeListGetString(node->doc, attr->children, 1);
            //     format = std::string((char*)value);
            //     //xmlFree(value);
            //   }
            // }

            assert(ofs != size_t(-1) && "Offset not properly parsed for Texture2D nodes");
            assert(width != size_t(-1) && "Width not properly parsed for Texture2D nodes");
            assert(height != size_t(-1) && "Height not properly parsed for Texture2D nodes");
            assert(channels != size_t(-1) && "Channel count not properly parsed for Texture2D nodes");
            assert(depth != size_t(-1) && "Depth not properly parsed for Texture2D nodes");
            assert( strcmp(format.c_str(), "") != 0 && "Format not properly parsed for Texture2D nodes");

#if 1
            throw std::runtime_error("cannot implelent textures yet");
#else
            txt.ptr->texData->channels = channels;
            txt.ptr->texData->depth = depth;
            txt.ptr->texData->width = width;
            txt.ptr->texData->height = height;
            txt.ptr->texData->data = (char*)(binBasePtr+ofs);
#endif
            // -------------------------------------------------------
          } else if (nodeName == "Material") {
            // -------------------------------------------------------
            Ref<sg::Material> mat = new sg::Material;
            nodeList.push_back(mat.ptr);

            // rivl::Material *mat = RIVLmat.ptr->general.ptr;

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
                  //This *could* be a texture, handle it!
                  if(childName.find("map_") == std::string::npos) {
                    mat->setParam(childName, (int32)atol(s)); 
                  } else {
                    throw std::runtime_error("no textures, yet ...");
                    // Texture2D* tex = mat->textures[(int32)atol(s)].ptr;
                    // mat->setParam(childName.c_str(), (void*)tex, Material::Param::TEXTURE);
                  }
                } else if (!childType.compare("int2")) {
                  int32 x = atol(s);
                  s = NEXT_TOK;
                  int32 y = atol(s);
                  // mat->setParam(childName.c_str(), vec2i(x,y));
                  mat->setParam(childName, vec2i(x,y));
                } else if (!childType.compare("int3")) {
                  int32 x = atol(s);
                  s = NEXT_TOK;
                  int32 y = atol(s);
                  s = NEXT_TOK;
                  int32 z = atol(s);
                  mat->setParam(childName, vec3i(x,y,z));
                } else if (!childType.compare("int4")) {
                  int32 x = atol(s);
                  s = NEXT_TOK;
                  int32 y = atol(s);
                  s = NEXT_TOK;
                  int32 z = atol(s);
                  s = NEXT_TOK;
                  int32 w = atol(s);
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
                  //xmlFree(value); 
#if 1
                  throw std::runtime_error("no textures yet");
#else
                  char *s = strtok(tokenBuffer, " \t\n\r");
                  while (s) {
                    int texID = atoi(s);
                    RIVLTexture * tex = nodeList[texID].cast<RIVLTexture>().ptr;
                    mat->textures.push_back(tex->texData);
                    s = NEXT_TOK;
                  }
#endif
                  free(tokenBuffer);
                }
#if 1
                  throw std::runtime_error("no textures yet");
#else
                if (mat->textures.size() != num) {
                  FATAL("invalid number of textures in material "
                        "(found either more or less than the 'num' field specifies");
                }
#endif
              }
            }
#undef NEXT_TOK
            // -------------------------------------------------------
          } else if (nodeName == "Transform") {
            // -------------------------------------------------------
            Ref<rivl::Transform> xfm = new rivl::Transform;
            nodeList.push_back(xfm.ptr);

            // find child ID
            for (int pID=0;pID<node->prop.size();pID++) {
              xml::Prop *prop = node->prop[pID];
              // for (xmlAttr* attr = node->properties; attr; attr = attr->next)
              if (prop->name == "child") {
                // xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                size_t childID = atoi(prop->value.c_str());//(char*)value);
                rivl::Node *child = (rivl::Node *)nodeList[childID].ptr;
                assert(child);
                xfm->child = child;
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
              FATAL("invalid number of elements in RIVL transform node");
            }
          
            // -------------------------------------------------------
          } else if (nodeName == "Mesh") {
            // -------------------------------------------------------
            Ref<rivl::TriangleMesh> mesh = new rivl::TriangleMesh;
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
                mesh->numVertices = num;
                mesh->vertex = (vec3f*)(binBasePtr+ofs);
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
                mesh->numNormals = num;
                mesh->normal = (vec3f*)(binBasePtr+ofs);
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
                mesh->numTexCoords = num;
                mesh->texCoord = (vec2f*)(binBasePtr+ofs);
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
                mesh->numTriangles = num;
                mesh->triangle = (vec4i*)(binBasePtr+ofs);
              } else if (childType == "materiallist") {
                char* value = strdup(child->content.c_str()); //xmlNodeListGetString(node->doc, child->children, 1);
                // xmlChar* value = xmlNodeListGetString(node->doc, child->children, 1);
                for(char *s=strtok((char*)value," \t\n\r");s;s=strtok(NULL," \t\n\r")) {
                  size_t matID = atoi(s);
                  Ref<sg::Material> mat = nodeList[matID].cast<sg::Material>();
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
            Ref<rivl::Group> group = new rivl::Group;
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
                rivl::Node *child = (rivl::Node*)nodeList[childID].ptr;
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
        return lastNode;
      }

      Ref<rivl::Node> importRIVL(const std::string &fileName)
      {
        string xmlFileName = fileName;
        string binFileName = fileName+".bin";

        FILE *file = fopen(binFileName.c_str(),"r");
        if (!file)
          perror("could not open binary file");
        fseek(file,0,SEEK_END);
        size_t fileSize = ftell(file);
        fclose(file);
      
        int fd = ::open(binFileName.c_str(),O_LARGEFILE|O_RDWR);
        if (fd == -1)
          perror("could not open file");
        binBasePtr = (unsigned char *)mmap(NULL,fileSize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

        Ref<xml::XMLDoc> doc = xml::readXML(fileName);
        if (doc->child.size() != 1 || doc->child[0]->name != "BGFscene") 
          throw std::runtime_error("could not parse RIVL file: Not in RIVL format!?");
        xml::Node *root_element = doc->child[0];
        Ref<Node> node = parseBGFscene(root_element);
        return node;
      }


    struct Triangle {
      uint32 v0, v1, v2;
      //      uint32 materialID; // iw, 1/11/14: disabled materialID
      //      per triangle, emrbee cannot do buffer sharing with this
      //      format.
    };

    /*! default triangle mesh layout */
    struct Mesh : public RefCount {
      std::string           name;     /*!< symbolic name of mesh, can be empty */
      std::vector<vec3fa>   position; /*!< vertex positions */
      std::vector<vec3fa>   normal;   /*!< vertex normals; empty if none present */
      std::vector<vec2f>    texcoord; /*!< vertex texcoords; empty if none present */
      std::vector<Triangle> triangle; /*!< triangles' vertex IDs */
      std::vector<Ref<sg::Material> > materialList; /*!< entire list of
                                             materials, in case the
                                             mesh has per-primitive
                                             material IDs (list may be
                                             empty, in which case the
                                             'material' field
                                             applies */

      /*! per-primitive material ID, if applicable. if empty every
          triangle uses the 'material' field of the mesh; if not emtpy
          the ID in here refers as a index into the
          'materialList'. Will eventually get merged into the foruth
          component of the triangle, but right now ospray/embree do
          not yet allow this ... */
      std::vector<uint32> triangleMaterialId;
      
      
      box3f bounds; /*!< bounding box of all vertices */

      int size() const { return triangle.size(); }
      Ref<sg::Material> material;
      box3f getBBox();
      Mesh() : bounds(embree::empty) {};
    };

    struct Instance : public RefCount {
      affine3f xfm;
      int meshID;
      
      Instance(int meshID=0, affine3f xfm=embree::one) : meshID(meshID), xfm(xfm) {};
    };
    bool operator==(const Instance &a, const Instance &b);
    bool operator!=(const Instance &a, const Instance &b);



    struct Model : public RefCount {
      // /*! list of materials - if per-triangle material IDs are used,
      //     then all material IDs of all meshes reference into this
      //     list */
      // std::vector<Ref<Material> > material;
      /*! list of meshes that the scene is composed of */
      std::vector<Ref<Mesh> >     mesh;
      /*! \brief list of instances (if available). */
      std::vector<Instance>      instance;

      //! return number of meshes in this model
      inline size_t numMeshes() const { return mesh.size(); }
      //! return number of unique triangles (ie, _ex_ instantiation!) in this model
      size_t numUniqueTriangles() const;
      /*! computes and returns the world-space bounding box of the entire model */
      box3f getBBox();
    };



      void traverseSG(Model &model, Ref<rivl::Node> &node, const affine3f &xfm=embree::one)
      {
        Group *g = dynamic_cast<Group *>(node.ptr);
        if (g) {
          for (int i=0;i<g->child.size();i++)
            traverseSG(model,g->child[i],xfm);
          return;
        }

        Transform *xf = dynamic_cast<Transform *>(node.ptr);
        if (xf) {
          traverseSG(model,xf->child,xfm*xf->xfm);
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
            for (int i=0;i<tm->numTriangles;i++) {
              Triangle t;
              t.v0 = tm->triangle[i].x;
              t.v1 = tm->triangle[i].y;
              t.v2 = tm->triangle[i].z;
              mesh->triangle[i] = t;
              mesh->triangleMaterialId[i] = tm->triangle[i].w >>16;
              if (mesh->triangleMaterialId[i]) anyNotZero = true;
            }
            if (!anyNotZero)
              mesh->triangleMaterialId.clear();

            for (int i=0;i<tm->numVertices;i++) {
              mesh->position[i].x = tm->vertex[i].x;
              mesh->position[i].y = tm->vertex[i].y;
              mesh->position[i].z = tm->vertex[i].z;
              mesh->position[i].w = 0;
            }
            if (tm->numNormals > 0) {
              // assert(tm->numNormals == tm->numVertices);
              mesh->normal.resize(tm->numVertices);
              for (int i=0;i<tm->numNormals;i++) {
                mesh->normal[i].x = tm->normal[i].x;
                mesh->normal[i].y = tm->normal[i].y;
                mesh->normal[i].z = tm->normal[i].z;
                mesh->normal[i].w = 0;
              }
            }
            if (tm->numTexCoords > 0) {
              // assert(tm->numTexCoords == tm->numVertices);
              mesh->texcoord.resize(tm->numVertices);
              for (int i=0; i<tm->numTexCoords; i++) {
                (vec2f&)mesh->texcoord[i] = (vec2f&)tm->texCoord[i];
              }
            }
            model.mesh.push_back(mesh);

            if (tm->material.size() == 1) {
              mesh->material = tm->material[0].ptr;
            } else {
              for (size_t i = 0; i < tm->material.size(); i++) {
                mesh->materialList.push_back(tm->material[i].ptr);
              }
            }
          }

          int meshID = meshIDs[tm];
          model.instance.push_back(Instance(meshID,xfm));
        
          return;
          // throw std::runtime_error("meshes not yet implemented in importRIVL");
        }

        sg::Material *mt = dynamic_cast<sg::Material *>(node.ptr);
        if (mt) {
          // model.material.push_back(mt->general);
          return;
        }

        //throw std::runtime_error("unhandled node type '"+node->toString()+"' in traverseSG");
      }
    } // ::ospray::viewer::rivl
    
    namespace sg {
      
      World *importRIVL(const std::string &fileName)
      {
        World *world = new World;
        
        Ref<rivl::Node> sg = rivl::importRIVL(fileName);
        Ref<sg::Model> model = new sg::Model;

        rivl::Model rivlModel;

        world->model = model;
        
        rivl::nodeList.clear();
        traverseSG(rivlModel,sg);
        rivl::nodeList.clear();

        return world;
      }
      
    } // ::ospray::viewer::sg
  } // ::ospray::viewer
} // ::ospray

