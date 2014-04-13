#undef NDEBUG

// header
#include "ospray/common/managed.h"
#include "ospray/common/data.h"
#include "miniSG.h"
// stl
#include <map>
// libxml
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlreader.h>
// stdlib, for mmap
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

namespace ospray {
  namespace miniSG {

    using std::string;

    //! base pointer to mmapped binary file (that all offsets are relative to)
    unsigned char *binBasePtr = NULL;

    /*! Base class for all scene graph node types */
    struct Node : public embree::RefCount
    {
      virtual string toString() const { return "ospray::miniSG::Node"; } 

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

    /*! Abstraction for a 'material' that a renderer can query from
      any geometry (it's up to the renderer to know what to do with
      the respective mateiral type) */
    struct RIVLMaterial : public miniSG::Node {
      virtual string toString() const { return "ospray::miniSG::RIVLMaterial"; } 
    };

    /*! Scene graph grouping node */
    struct Group : public miniSG::Node {
      virtual string toString() const;
      std::vector<Ref<miniSG::Node> > child;
    };

    /*! scene graph node that contains an ospray geometry type (i.e.,
      anything that defines some sort of geometric surface */
    struct Geometry : public miniSG::Node {
      std::vector<Ref<miniSG::RIVLMaterial> > material;
      virtual string toString() const { return "ospray::miniSG::Geometry"; } 
    };

    /*! scene graph node that contains an ospray geometry type (i.e.,
      anything that defines some sort of geometric surface */
    struct Texture : public miniSG::Node {
      virtual string toString() const { return "ospray::miniSG::Texture"; } 
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

      /*! \brief data handle to vertex data array. 
        
        can be null if user set the 'vertex' pointer manually */
      Ref<ospray::Data> vertexData;
      /*! \brief data handle to normal data array. 

        can be null if user set the 'normal' pointer manually */
      Ref<ospray::Data> normalData;
      /*! \brief data handle to txcoord data array. 

        can be null if user set the 'txcoord' pointer manually */
      Ref<ospray::Data> texCoordData;
      /*! \brief data handle to index data array. 

        can be null if user set the 'index' pointer manually */
      Ref<ospray::Data> indexData;

      vec4i *triangle;
      size_t numTriangles;
      vec3f *vertex;
      size_t numVertices;
      vec3f *normal;
      size_t numNormals;
      vec3f *texCoord;
      size_t numTexCoords;
    };

    std::vector<Ref<miniSG::Node> > nodeList;

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
    
    Ref<miniSG::Node> parseBGFscene(xmlNode *root)
    {
      std::string rootName = (char*)root->name;
      if (rootName != "BGFscene")
        throw std::runtime_error("XML file is not a RIVL model !?");
      if (root->children == NULL)
        throw std::runtime_error("emply RIVL model !?");

      Ref<miniSG::Node> lastNode;
      for (xmlNode *node = root->children; node; node = node->next) {
        std::string nodeName = (char*)node->name;
        if (nodeName == "text") {
          //PRINT(nodeName);
          // -------------------------------------------------------
        } else if (nodeName == "Texture2D") {
          // -------------------------------------------------------
          Ref<miniSG::Texture> txt = new miniSG::Texture;
          nodeList.push_back(txt.ptr);

          static int warnCount = 0;
          if (!warnCount++)
            std::cout << "warning: parsing RIVL Texture2D not yet implemented "
              "- creating dummy texture" << std::endl;
          // -------------------------------------------------------
        } else if (nodeName == "Material") {
          // -------------------------------------------------------
          Ref<miniSG::RIVLMaterial> mat = new miniSG::RIVLMaterial;
          nodeList.push_back(mat.ptr);

          static int warnCount = 0;
          if (!warnCount++)
            std::cout << "warning: parsing RIVL Material not yet implemented "
              "- creating dummy material" << std::endl;
          // -------------------------------------------------------
        } else if (nodeName == "Transform") {
          // -------------------------------------------------------
          Ref<miniSG::Transform> xfm = new miniSG::Transform;
          nodeList.push_back(xfm.ptr);

          // find child ID
          for (xmlAttr* attr = node->properties; attr; attr = attr->next)
            if (!strcmp((const char*)attr->name,"child")) {
              xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
              size_t childID = atoi((char*)value);
              miniSG::Node *child = nodeList[childID].ptr;
              assert(child);
              xfm->child = child;
              xmlFree(value); 
            }       

          // parse xfm matrix
          xmlChar* value = xmlNodeListGetString(node->doc, node->children, 1);
          sscanf((char*)value,"%f %f %f\n%f %f %f\n%f %f %f\n%f %f %f",
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
          xmlFree(value);

          // -------------------------------------------------------
        } else if (nodeName == "Mesh") {
          // -------------------------------------------------------
          Ref<miniSG::TriangleMesh> mesh = new miniSG::TriangleMesh;
          nodeList.push_back(mesh.ptr);

          for (xmlNode *child=node->children;child;child=child->next) {
            std::string childType = (char*)child->name;
            if (childType == "text") {
            } else if (childType == "vertex") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (!strcmp((const char*)attr->name,"ofs")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol((char*)value);
                  xmlFree(value); 
                }       
                else if (!strcmp((const char*)attr->name,"num")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol((char*)value);
                  xmlFree(value); 
                }       
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
            } else if (childType == "normal") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (!strcmp((const char*)attr->name,"ofs")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol((char*)value);
                  xmlFree(value); 
                }       
                else if (!strcmp((const char*)attr->name,"num")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol((char*)value);
                  xmlFree(value); 
                }       
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
            } else if (childType == "texcoord") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (!strcmp((const char*)attr->name,"ofs")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol((char*)value);
                  xmlFree(value); 
                }       
                else if (!strcmp((const char*)attr->name,"num")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol((char*)value);
                  xmlFree(value); 
                }       
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
            } else if (childType == "prim") {
              size_t ofs = -1, num = -1;
              // scan parameters ...
              for (xmlAttr* attr = child->properties; attr; attr = attr->next)
                if (!strcmp((const char*)attr->name,"ofs")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  ofs = atol((char*)value);
                  xmlFree(value); 
                }       
                else if (!strcmp((const char*)attr->name,"num")) {
                  xmlChar* value = xmlNodeListGetString(node->doc, attr->children, 1);
                  num = atol((char*)value);
                  xmlFree(value); 
                }       
              assert(ofs != size_t(-1));
              assert(num != size_t(-1));
              mesh->numTriangles = num;
              mesh->triangle = (vec4i*)(binBasePtr+ofs);
            } else if (childType == "materiallist") {
              xmlChar* value = xmlNodeListGetString(node->doc, node->children, 1);
              for(char *s=strtok((char*)value," \t\n");s;s=strtok(NULL," \t\n")) {
                size_t matID = atoi(s);
                miniSG::RIVLMaterial *mat = nodeList[matID].cast<miniSG::RIVLMaterial>().ptr;
                assert(mat);
                mesh->material.push_back(mat);
              }
              xmlFree(value);
            } else {
              throw std::runtime_error("unknown child node type '"+childType+"' for mesh node");
            }
          }

          lastNode = mesh.ptr;
          // -------------------------------------------------------
        } else if (nodeName == "Group") {
          // -------------------------------------------------------
          Ref<miniSG::Group> group = new miniSG::Group;
          nodeList.push_back(group.ptr);

          xmlChar* value = xmlNodeListGetString(node->doc, node->children, 1);
          for(char *s=strtok((char*)value," \t\n");s;s=strtok(NULL," \t\n")) {
            size_t childID = atoi(s);
            miniSG::Node *child = nodeList[childID].ptr;
            assert(child);
            group->child.push_back(child);
          }
          xmlFree(value);
          lastNode = group.ptr;
        } else {
          throw std::runtime_error("unknown node type '"+nodeName+"' in RIVL model");
        }
      }
      return lastNode;
    }

    Ref<miniSG::Node> importRIVL(const std::string &fileName)
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
      /*
       * this initialize the library and check potential ABI mismatches
       * between the version it was compiled for and the actual shared
       * library used.
       */
      LIBXML_TEST_VERSION;
      
      xmlDocPtr doc; /* the resulting document tree */
      
      doc = xmlReadFile(xmlFileName.c_str(), NULL, 0);
      if (doc == NULL) 
        throw std::runtime_error("could not open/parse xml file '"
                                 +xmlFileName+"'");

      xmlNode * root_element = xmlDocGetRootElement(doc);
      Ref<Node> node = parseBGFscene(root_element);

      xmlFreeDoc(doc);
      return node;
    }

    void traverseSG(Model &model, Ref<miniSG::Node> &node, const affine3f &xfm=embree::one)
    {
      Group *g = dynamic_cast<Group *>(node.ptr);
      if (g) {
        for (int i=0;i<g->child.size();i++)
          traverseSG(model,g->child[i],xfm);
        return;
      }

      TriangleMesh *tm = dynamic_cast<TriangleMesh *>(node.ptr);
      if (tm) {
        throw std::runtime_error("meshes not yet implemented in importRIVL");
      }

      throw std::runtime_error("unhandled node type '"+node->toString()+"' in traverseSG");
    }

    /*! import a wavefront OBJ file, and add it to the specified model */
    void importRIVL(Model &model, const embree::FileName &fileName)
    {
      Ref<miniSG::Node> sg = importRIVL(fileName);
      traverseSG(model,sg);

      NOTIMPLEMENTED;
    }
    
  }
}
