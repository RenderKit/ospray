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

// ospcommon
#include "ospcommon/containers/AlignedVector.h"
// sg
#include "SceneGraph.h"
#include "sg/geometry/TriangleMesh.h"
#include "sg/texture/Texture2D.h"
// xml lib
#include "ospcommon/xml/XML.h"
// std
#include <fstream>
#include <set>

namespace ospray {
  namespace sg {

    void warnIgnore(const std::string &nodeType)
    {
      static std::set<std::string> alreadyWarned;
      if (alreadyWarned.find(nodeType) != alreadyWarned.end())
        return;

      alreadyWarned.insert(nodeType);
      std::cout << "#importer::X3D: ignoring node type " << nodeType
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

    void parseVectorOfVec3fas(containers::AlignedVector<vec3fa> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      char *tok = strtok(s,delim);
      while (tok)
        vec.push_back(parseVec3f(tok));
      free(s);
    }

    void parseVectorOfVec3fs(containers::AlignedVector<vec3f> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      char *tok = strtok(s,delim);
      while (tok)
        vec.push_back(parseVec3f(tok));
      free(s);
    }

    void parseVectorOfColors(containers::AlignedVector<vec4f> &vec, const std::string &str)
    {
      char *s = strdup(str.c_str());
      char *tok = strtok(s,delim);
      while (tok)
        vec.push_back(vec4f(parseVec3f(tok), 1.f));
      free(s);
    }

    void parseIndexedFaceSet(const std::shared_ptr<Node> model,
                             const affine3f &/*xfm*/, const xml::Node &root)
    {
      std::string name = "x3d mesh";
      auto mesh = createNode(name, "TriangleMesh")->nodeAs<TriangleMesh>();
      auto v = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
      mesh->add(v);

      auto vi = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
      mesh->add(vi);

      // -------------------------------------------------------
      // parse coordinate indices
      // -------------------------------------------------------
      std::string coordIndex = root.getProp("coordIndex");
      assert(coordIndex != "");

      char *s = strdup(coordIndex.c_str()); coordIndex = "";
      char *tok = strtok(s,delim);
      containers::AlignedVector<int> ID;
      while (tok) {
        long thisID = atol(tok);
        if (thisID == -1) {
          for (size_t i = 2; i < ID.size(); i++) {
            vec3i t;
            t.x = ID[0];
            t.y = ID[i-1];
            t.z = ID[i];
            vi->v.push_back(t);
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
      for (auto &node : root.child) {
        if (node.name == "Coordinate") {
          parseVectorOfVec3fs(v->v,node.getProp("point"));
        } else if (node.name == "Normal") {
          parseVectorOfVec3fs(v->v,node.getProp("vector"));
        } else if (node.name == "Color") {
          // parseVectorOfColors(mesh->color,node.getProp("color"));
        } else {
          std::cout << "importX3D: unknown child type '" << node.name
                    << "' to 'IndexedFaceSet' node\n";
        }
      }

      model->add(mesh);
    }

    void parseShape(const std::shared_ptr<Node> model,
                    const affine3f &xfm,
                    const xml::Node &root)
    {
      for (auto &node : root.child) {
        if (node.name == "Appearance") {
          /* ignore for now */
          warnIgnore("'Appearance' (in Shape)");
        } else if (node.name == "IndexedLineSet") {
          /* ignore for now */
          warnIgnore("'IndexedLineSet' (in Shape)");
        } else if (node.name == "IndexedFaceSet") {
          /* ignore for now */
          parseIndexedFaceSet(model,xfm,node);
        } else {
          throw std::runtime_error("importX3D: unknown child type '"
                                   + node.name + "' to 'Shape' node");
        }
      }
    }

    void parseGroup(const std::shared_ptr<Node> model,
                    const affine3f &xfm,
                    const xml::Node &root)
    {
      for (auto &node : root.child) {
        if (node.name == "Appearance") {
          /* ignore for now */
          warnIgnore("'Appearance' (in Shape)");
        } else if (node.name == "Shape") {
          parseShape(model,xfm,node);
        } else {
          throw std::runtime_error("importX3D: unknown child type '"+node.name+"' to 'Group' node");
        }
      }
    }

    void parseTransform(const std::shared_ptr<Node> model,
                        const affine3f &parentXFM, const xml::Node &root)
    {
      affine3f xfm = parentXFM;

      for (auto &node : root.child) {
        if (node.name == "DirectionalLight") {
          /* ignore */
          warnIgnore("'DirectionalLight' (in Transform node)");
        } else if (node.name == "Transform") {
          parseTransform(model,xfm,node);
        } else if (node.name == "Shape") {
          parseShape(model,xfm,node);
        } else if (node.name == "Group") {
          parseGroup(model,xfm,node);
        } else {
          throw std::runtime_error("importX3D: unknown 'transform' child type '"
                                   + node.name + "'");
        }
      }
    }

    void parseX3D(const std::shared_ptr<Node> model, const xml::Node &root)
    {
      for (auto &node : root.child) {
        if (node.name == "Background") {
          /* ignore */
          warnIgnore("'Background' (in root node)");
        } else if (node.name == "Viewpoint") {
          /* ignore */
          warnIgnore("'Viewpoint' (in root node)");
        } else if (node.name == "NavigationInfo") {
          /* ignore */
          warnIgnore("'NavigationInfo' (in root node)");
        } else if (node.name == "DirectionalLight") {
          /* ignore */
          warnIgnore("'DirectionalLight' (in root node)");
        } else if (node.name == "Transform") {
          affine3f xfm = ospcommon::one;
          parseTransform(model,xfm,node);
          /* ignore */
        } else {
          throw std::runtime_error("importX3D: unknown node type '"+node.name+"'");
        }
      }
    }

    /*! import an X3D model, as forexample a ParaView contour exported
      using ParaView's X3D exporter */
    OSPSG_INTERFACE void importX3D(const std::shared_ptr<Node> &world,
                                   const FileName &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fileName);
      assert(doc);
      const xml::Node &root_element = doc->child[0];
      PRINT(root_element.name);
      for (auto &node : root_element.child) {
        if (node.name == "head") {
          /* ignore meta data for now */
        } else if (node.name == "Scene") {
          parseX3D(world,node);
        } else {
          throw std::runtime_error("unknown X3D root element "+node.name);
        }
      }
    }

    OSPSG_REGISTER_IMPORT_FUNCTION(importX3D, x3d);

  }  // ::ospray::sg
}  // ::ospray
