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

// run the parser with assertions, even in release mode:
#undef NDEBUG

#include "miniSG.h"
#include "importer.h"
// xml lib
#include "apps/common/xml/XML.h"
// std
#include <fstream>
#include <set>

namespace ospray {
  namespace miniSG {

    using std::cout;
    using std::endl;

    void warnIgnore(const std::string &nodeType)
    {
      static std::set<std::string> alreadyWarned;
      if (alreadyWarned.find(nodeType) != alreadyWarned.end()) return;
      alreadyWarned.insert(nodeType);
      std::cout << "#miniSG::X3D: ignoring node type " << nodeType 
                << " (more instances may follow)"
                << std::endl;
      
    }
    void parseVectorOfVec3fas(std::vector<vec3fa> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      const char *delim = "\n\t\r, ";
      char *tok = strtok(s,delim);
      while (tok) {
        vec3fa v;
        v.x = atof(tok);

        tok = strtok(NULL,delim);
        if (!tok) break;
        v.y = atof(tok);
        
        tok = strtok(NULL,delim);
        if (!tok) break;
        v.z = atof(tok);

        vec.push_back(v);

        tok = strtok(NULL,delim);
      }
      free(s);
    }
    void parseIndexedFaceSet(Model &model, const affine3f &xfm, xml::Node *root)
    {
      Ref<Mesh> mesh = new Mesh;
      mesh->material = new Material;

      // -------------------------------------------------------
      // parse coordinate indices
      // -------------------------------------------------------
      std::string coordIndex = root->getProp("coordIndex");
      assert(coordIndex != "");

      char *s = strdup(coordIndex.c_str()); coordIndex = "";
      const char *delim = "\n\t\r, ";
      char *tok = strtok(s,delim);
      std::vector<int> ID;
      while (tok) {
        long thisID = atol(tok);
        if (thisID == -1) {
          for (int i=2;i<ID.size();i++) {
            Triangle t;
            t.v0 = ID[0];
            t.v1 = ID[i-1];
            t.v2 = ID[i];
            mesh->triangle.push_back(t);
          }
          ID.clear();
        } else {
          ID.push_back(thisID);
        }
        tok = strtok(NULL,delim);
      }
      free(s);

      // -------------------------------------------------------
      // now, parse children for vertex arrays
      // -------------------------------------------------------
      for (int childID=0;childID<root->child.size();childID++) {
        xml::Node *node = root->child[childID];
        
        if (node->name == "Coordinate") {
          parseVectorOfVec3fas(mesh->position,node->getProp("point"));
          continue;
        }
        if (node->name == "Normal") {
          parseVectorOfVec3fas(mesh->normal,node->getProp("vector"));
          continue;
        }
        if (node->name == "Color") {
          /* ignore for now */
          parseVectorOfVec3fas(mesh->color,node->getProp("color"));
          // warnIgnore("'Color' (in IndexedFaceSet)");
          continue;
        }
        warnIgnore("'" + node->name + "' child of 'IndexedFaceSet' node");
      }

      model.mesh.push_back(mesh);
      model.instance.push_back(Instance(model.mesh.size()-1,xfm));
    }
    void parseShape(Model &model, const affine3f &xfm, xml::Node *root)
    {
      for (int childID=0;childID<root->child.size();childID++) {
        xml::Node *node = root->child[childID];
        
        if (node->name == "Appearance") {
          /* ignore for now */
          warnIgnore("'Appearance' (in Shape)");
          continue;
        }
        if (node->name == "IndexedLineSet") {
          /* ignore for now */
          warnIgnore("'IndexedLineSet' (in Shape)");
          continue;
        }
        if (node->name == "IndexedFaceSet") {
          /* ignore for now */
          parseIndexedFaceSet(model,xfm,node);
          continue;
        }

        throw std::runtime_error("importX3D: unknown child type '"+node->name+"' to 'Shape' node");
      }
    }
    void parseTransform(Model &model, const affine3f &parentXFM, xml::Node *root)
    {
      affine3f xfm = parentXFM;

      // TODO: parse actual xfm parmeters ...
      for (int childID=0;childID<root->child.size();childID++) {
        xml::Node *node = root->child[childID];
        
        if (node->name == "DirectionalLight") {
          /* ignore */
          warnIgnore("'DirectionalLight' (in Transform node)");
          continue;
        }
        if (node->name == "Transform") {
          parseTransform(model,xfm,node);
          continue;
        }
        if (node->name == "Shape") {
          parseShape(model,xfm,node);
          continue;
        }

        throw std::runtime_error("importX3D: unknown 'transform' child type '"+node->name+"'");
      }
    }
    void parseX3D(Model &model, xml::Node *root)
    {
      assert(root->child.size() == 2);
      assert(root->child[0]->name == "head");
      assert(root->child[1]->name == "Scene");
      xml::Node *sceneNode = root->child[1];
      for (int childID=0;childID<sceneNode->child.size();childID++) {
        xml::Node *node = sceneNode->child[childID];
        if (node->name == "Background") {
          /* ignore */
          warnIgnore("'Background' (in root node)");
          continue;
        }
        if (node->name == "Viewpoint") {
          /* ignore */
          warnIgnore("'Viewpoint' (in root node)");
          continue;
        }
        if (node->name == "NavigationInfo") {
          /* ignore */
          warnIgnore("'NavigationInfo' (in root node)");
          continue;
        }
        if (node->name == "DirectionalLight") {
          /* ignore */
          warnIgnore("'DirectionalLight' (in root node)");
          continue;
        }
        if (node->name == "Transform") {
          affine3f xfm = ospcommon::one;
          parseTransform(model,xfm,node);
          /* ignore */
          continue;
        }
        throw std::runtime_error("importX3D: unknown node type '"+node->name+"'");
      }
    }

    /*! import a list of X3D files */
    void importX3D(Model &model, 
                   const ospcommon::FileName &fileName)
    {
      xml::XMLDoc *doc = xml::readXML(fileName);
      assert(doc);
      PRINT(doc->child[0]->name);
      if (doc->child.size() != 1 || doc->child[0]->name != "X3D") 
        throw std::runtime_error("could not parse X3D file: Not in X3D format!?");
      xml::Node *root_element = doc->child[0];
      parseX3D(model,root_element);
    }

  } // ::ospray::minisg
} // ::ospray
