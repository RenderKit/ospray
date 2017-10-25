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

#include "SceneGraph.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    std::shared_ptr<sg::Node> parseXMLNode(const xml::Node &node,
                      const unsigned char *binBasePtr, std::shared_ptr<sg::Node> sgNode)
    {
      std::string type = node.getProp("type");
      std::string value = node.getProp("value");
      if (value == "")
        value = std::string(node.content);
      if (type == "")
        type = "Node";
      std::stringstream ss(value);
      if (type == "float")
        sgNode->setValue(std::stof(value));
      else if (type == "int")
        sgNode->setValue(std::stoi(value));
      else if (type == "string")
      {
        sgNode->setValue(value);
      }
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
      else if (type == "DataVector3f")
      {
        vec3f val;
        while (ss.good()) {
          ss >> val.x >> val.y >> val.z;
          sgNode->nodeAs<DataVector3f>()->push_back(val);
        }
      }
      for (auto child : node.child)
      {
        if (sgNode->hasChild(child->name))
        {
          parseXMLNode(*child, binBasePtr, sgNode->child(child->name).shared_from_this());
        }
        else
        {
          const std::string& cname = child->name;
          const std::string ctype = child->getProp("type");
          const std::string cname2 = child->getProp("nodeName"); // for node references
          if (cname2 != "")
          {
            auto newNode = sg::createNode(cname2,ctype);
            sgNode->setChild(cname, newNode);
            newNode->setParent(sgNode);
            parseXMLNode(*child, binBasePtr, newNode);
          }
          else
            sgNode->add(parseXMLNode(*child, binBasePtr, sg::createNode(cname,ctype)));
        }
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

      std::shared_ptr<Node> node = world;
      if (root->name != world->name())
      {
        try
        {
          node = world->childRecursive(root->name).shared_from_this();
        }
        catch (const std::runtime_error &)
        {
          std::string type = root->getProp("type");
          if (type == "")
            type = "Node";
          node = sg::createNode(root->name, type);
          world->add(node);
        }
      }

      parseXMLNode(*root, binBasePtr, node);
      std::cout << "loaded xml: \n";
      node->traverse("print");
      node->traverse("modified");
    }

    void writeNode(const std::string ptrName, const std::shared_ptr<sg::Node> &node, FILE* out, const int indent)
    {
      for (int i=0;i<indent;i++)
        fprintf(out,"  ");
      const std::string type = node->type();
      std::string lower=node->name();
      std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
      if (ptrName != lower)
        fprintf(out, "<%s nodeName=\"%s\" type=\"%s\"",ptrName.c_str(), node->name().c_str(), type.c_str());
      else
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
        writeNode(child.first, child.second, out, indent+1);
      }
      if (!node->children().empty())
      {
        for (int i = 0; i < indent; i++)
          fprintf(out,"  ");
      }
      if (ptrName != lower)
        fprintf(out, "</%s>\n",ptrName.c_str());
      else
        fprintf(out, "</%s>\n",node->name().c_str());
    }

    void writeOSPSG(const std::shared_ptr<sg::Node> &root,
                                   const std::string &fileName)
    {
      FILE *file = fopen(fileName.c_str(),"w");
      if (!file) {
        throw std::runtime_error("ospray::XML error: could not open file for writing '"
                                 + fileName +"'");
      }
      FILE *fbin = fopen((fileName+".bin").c_str(),"w");
      if (!fbin) {
        throw std::runtime_error("ospray::XML error: could not open file for writing '"
                                 + fileName +"'");
      }
      fprintf(file,"<?xml version=\"%s\"?>\n","1.0");
      fprintf(file,"<!-- OSPRay Version 1.2.3 -->\n");
      writeNode(root->name(), root, file, 1);
      fclose(file);
      fclose(fbin);
    }

  } // ::ospray::sg
} // ::ospray
