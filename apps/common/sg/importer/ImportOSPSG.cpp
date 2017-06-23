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

    void loadOSPSG(const std::shared_ptr<Node> &world,
                                   const std::string &fileName)
    {

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
      fprintf(file,"<?xml version=\"%s\"?>\n","1.2.3");
      writeNode(root, file, 0);
    }

  } // ::ospray::sg
} // ::ospray
