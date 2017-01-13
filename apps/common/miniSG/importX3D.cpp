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

// run the parser with assertions, even in release mode:
#undef NDEBUG

#include "miniSG.h"
#include "importer.h"
// xml lib
#include "common/xml/XML.h"
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
    
    static const char *delim = "\n\t\r, ";

    vec3f parseVec3f(char * &tok)
    {
      vec3f v;
      v.x = atof(tok);

      tok = strtok(NULL,delim);
      if (!tok) return v;
      v.y = atof(tok);

      tok = strtok(NULL,delim);
      if (!tok) return v;
      v.z = atof(tok);

      tok = strtok(NULL,delim);
      return v;
    }

    void parseVectorOfVec3fas(std::vector<vec3fa> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      char *tok = strtok(s,delim);
      while (tok)
        vec.push_back(parseVec3f(tok));
      free(s);
    }
    
    void parseVectorOfColors(std::vector<vec4f> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      char *tok = strtok(s,delim);
      while (tok)
        vec.push_back(vec4f(parseVec3f(tok), 1.f));
      free(s);
    }
    
    void parseIndexedFaceSet(Model &model, const affine3f &xfm, const xml::Node &root)
    {
      Ref<Mesh> mesh = new Mesh;
      mesh->material = new Material;

      // -------------------------------------------------------
      // parse coordinate indices
      // -------------------------------------------------------
      std::string coordIndex = root.getProp("coordIndex");
      assert(coordIndex != "");

      char *s = strdup(coordIndex.c_str()); coordIndex = "";
      char *tok = strtok(s,delim);
      std::vector<int> ID;
      while (tok) {
        long thisID = atol(tok);
        if (thisID == -1) {
          for (size_t i = 2; i < ID.size(); i++) {
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
      xml::for_each_child_of(root,[&](const xml::Node &node){
          if (node.name == "Coordinate") {
            parseVectorOfVec3fas(mesh->position,node.getProp("point"));
          }
          else if (node.name == "Normal") {
            parseVectorOfVec3fas(mesh->normal,node.getProp("vector"));
          }
          else if (node.name == "Color") {
            parseVectorOfColors(mesh->color,node.getProp("color"));
          }
          else
            std::cout << "importX3D: unknown child type '" << node.name
                      << "' to 'IndexedFaceSet' node\n";
        });

      model.mesh.push_back(mesh);
      model.instance.push_back(Instance(model.mesh.size()-1,xfm));
    }
    
    void parseShape(Model &model, const affine3f &xfm, const xml::Node &root)
    {
      xml::for_each_child_of(root,[&](const xml::Node &node){
          if (node.name == "Appearance") {
            /* ignore for now */
            warnIgnore("'Appearance' (in Shape)");
          } else if (node.name == "IndexedLineSet") {
            /* ignore for now */
            warnIgnore("'IndexedLineSet' (in Shape)");
          } else if (node.name == "IndexedFaceSet") {
            /* ignore for now */
            parseIndexedFaceSet(model,xfm,node);
          } else
            throw std::runtime_error("importX3D: unknown child type '"+node.name+"' to 'Shape' node");
        });
    }
    
    void parseTransform(Model &model, const affine3f &parentXFM, const xml::Node &root)
    {
      affine3f xfm = parentXFM;

      // TODO: parse actual xfm parmeters ...
      xml::for_each_child_of(root,[&](const xml::Node &node){
          if (node.name == "DirectionalLight") {
            /* ignore */
            warnIgnore("'DirectionalLight' (in Transform node)");
          } else if (node.name == "Transform") {
            parseTransform(model,xfm,node);
          } else if (node.name == "Shape") {
            parseShape(model,xfm,node);
          } else throw std::runtime_error("importX3D: unknown 'transform' child type '"
                                          + node.name + "'");
        });
    }
    
    void parseX3D(Model &model, const xml::Node &root)
    {
      assert(root.child.size() == 2);
      assert(root.child[0]->name == "head");
      assert(root.child[1]->name == "Scene");
      const xml::Node &sceneNode = *root.child[1];
      xml::for_each_child_of(sceneNode,[&](const xml::Node &node){
          if (node.name == "Background") {
            /* ignore */
            warnIgnore("'Background' (in root node)");
          }
          else if (node.name == "Viewpoint") {
            /* ignore */
            warnIgnore("'Viewpoint' (in root node)");
          }
          else if (node.name == "NavigationInfo") {
            /* ignore */
            warnIgnore("'NavigationInfo' (in root node)");
          }
          else if (node.name == "DirectionalLight") {
            /* ignore */
            warnIgnore("'DirectionalLight' (in root node)");
          }
          else if (node.name == "Transform") {
            affine3f xfm = ospcommon::one;
            parseTransform(model,xfm,node);
            /* ignore */
          }
          else
            throw std::runtime_error("importX3D: unknown node type '"+node.name+"'");
        });
    }

    /*! import a list of X3D files */
    void importX3D(Model &model, 
                   const ospcommon::FileName &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fileName);
      assert(doc);
      PRINT(doc->child[0]->name);
      if (doc->child.size() != 1 || doc->child[0]->name != "X3D") 
        throw std::runtime_error("could not parse X3D file: Not in X3D format!?");
      const xml::Node &root_element = *doc->child[0];
      parseX3D(model,root_element);
    }

  } // ::ospray::minisg
} // ::ospray
