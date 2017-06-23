// // ======================================================================== //
// // Copyright 2009-2016 Intel Corporation                                    //
// //                                                                          //
// // Licensed under the Apache License, Version 2.0 (the "License");          //
// // you may not use this file except in compliance with the License.         //
// // You may obtain a copy of the License at                                  //
// //                                                                          //
// //     http://www.apache.org/licenses/LICENSE-2.0                           //
// //                                                                          //
// // Unless required by applicable law or agreed to in writing, software      //
// // distributed under the License is distributed on an "AS IS" BASIS,        //
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// // See the License for the specific language governing permissions and      //
// // limitations under the License.                                           //
// // ======================================================================== //

// #undef NDEBUG

// header
#include "SceneGraph.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    std::shared_ptr<sg::Node> parseXMLNode(const xml::Node &node,
                      const unsigned char *binBasePtr)
    {
      const std::string& name = node.name;
      std::string type = node.getProp("type");
      std::string value = node.getProp("value");
      if (value == "")
        value = std::string(node.content);
      if (type == "")
        type = "Node";
      std::cout << "parsing xml node: " << name << ":" << type << "=" << value << "\n";
      auto sgNode = sg::createNode(name,type);
      std::stringstream ss(value);
      if (type == "float")
        sgNode->setValue(std::stof(value));
      else if (type == "int")
        sgNode->setValue(std::stoi(value));
      else if (type == "string")
        sgNode->setValue(value);
      else if (type == "vec3f")
      {
        vec3f val;
        ss >> val.x >> val.y >> val.z;
        sgNode->setValue(val);
      }
      else if (type == "vec2i")
      {
        vec2i val;
        ss >> val.x >> val.y;
        sgNode->setValue(val);
      }
      for (auto child : node.child)
      {
        sgNode->add(parseXMLNode(*child, binBasePtr));
      }
      return sgNode;
    }

    void loadOSPSG(const std::shared_ptr<Node> &world,
                                   const std::string &fileName)
    {
      std::shared_ptr<xml::XMLDoc> doc;
      doc = xml::readXML(fileName);
      if (doc->child.empty())
        throw std::runtime_error("ospray xml input file does not contain any nodes!?");
      const std::string binFileName = fileName+"bin";
      const unsigned char * const binBasePtr = mapFile(binFileName);
      std::shared_ptr<xml::Node> root = doc->child[0];

      auto node = parseXMLNode(*root, binBasePtr);
      std::cout << "loaded xml: \n";
      node->traverse("print");
      world->add(node);
    }

    void writeNode(const std::shared_ptr<Node> &node, FILE* out, const int indent)
    {
      for (int i=0;i<indent;i++)
        fprintf(out,"  ");
      const std::string type = node->type();
    	fprintf(out, "<%s type=\"%s\"",node->name().c_str(), type.c_str());
    	if (node->children().empty())
    	  fprintf(out, ">");
    	else
        fprintf(out, " value=\"");
      // print value
      std::stringstream ss;
      if (type == "float")
        fprintf(out, "%f", node->valueAs<float>());
      else if (type == "int")
        fprintf(out, "%d", node->valueAs<int>());
      else if (type == "bool")
        fprintf(out, "%d", node->valueAs<bool>());
      else if (type == "string")
        fprintf(out, "%s", node->valueAs<std::string>().c_str());
      else if (type == "vec3f")
      {
        auto val = node->valueAs<vec3f>();
        fprintf(out, "%f %f %f", val.x,val.y,val.z);
      }
      else if (type == "vec2i")
      {
        auto val = node->valueAs<vec2i>();
        fprintf(out, "%d %d", val.x,val.y);
      }

      if (!node->children().empty())
        fprintf(out, "\">\n");

      for(auto child : node->children())
      {
        writeNode(child, out, indent+1);
      }
      if (!node->children().empty())
      {
        for (int i=0;i<indent;i++)
          fprintf(out,"  ");
      }
      fprintf(out, "</%s>\n",node->name().c_str());
    }

    void writeOSPSG(const std::shared_ptr<Node> &root,
                                   const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"w");
      if (!file) {
        throw std::runtime_error("ospray::XML error: could not open file for writing '"
                                 + fileName +"'");
      }
      FILE *fbin = fopen((fileName+".bin").c_str(),"w");
      if (!file) {
        throw std::runtime_error("ospray::XML error: could not open file for writing '"
                                 + fileName +"'");
      }
      fprintf(file,"<?xml version=\"%s\"?>\n","1.0");
      fprintf(file,"<!-- OSPRay Version 1.2.3 -->");
      writeNode(root, file, 1);
      fclose(file);
    }

  } // ::ospray::sg
} // ::ospray
