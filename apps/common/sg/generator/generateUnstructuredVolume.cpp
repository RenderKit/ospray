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
#include "ospcommon/utility/StringManip.h"
#include "ospcommon/multidim_index_sequence.h"
// sg
#include "../common/Data.h"
#include "../common/NodeList.h"
#include "Generator.h"

namespace ospray {
  namespace sg {

    void generateTetrahedrons(const std::shared_ptr<Node> &world,
                              const std::vector<string_pair> &params)
    {
      auto tets_node = createNode("unstructured_tetrahedrons",
                                  "UnstructuredVolume");

      // get generator parameters

      vec2i dims(100, 100);

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 2) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxY'"
                      << std::endl;
            continue;
          }

          dims = vec2i(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()));
        } else {
          std::cout << "WARNING: unknown tetrahedron generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate sphere data

      auto verts = createNode("vertices", "DataVector3f")->nodeAs<DataVector3f>();
      auto indices = createNode("indices", "DataVector4i")->nodeAs<DataVector4i>();
      auto field = createNode("field", "DataVector1f")->nodeAs<DataVector1f>();

      for (int w = 0; w < 2; w++) {
        for (int u = 0; u < dims.x; u++) {
          for (int v = 0; v < dims.y; v++) {
            float uu = M_PI * (-0.5f + u / float(dims.x - 1));
            float vv = M_PI * (v / float(dims.y - 1));
            float radius = 1.0f - float(w)/4;

            verts->push_back(radius * vec3f(std::cos(uu) * std::cos(vv),
                                            std::cos(uu) * std::sin(vv),
                                            std::sin(uu)));
            field->push_back(std::cos(4 * uu) * std::sin(4 *vv));
          }
        }
      }

      auto offset = dims.x * dims.y;
      for (int u = 0; u < dims.x - 1; u++) {
        for (int v = 0; v < dims.y - 1; v++) {
          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i(v * dims.x + u,
                                   v * dims.x + (u + 1),
                                   (v + 1) * dims.x + u,
                                   v * dims.x + u + offset));

          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i((v + 1) * dims.x + (u + 1),
                                   v * dims.x + (u + 1),
                                   (v + 1) * dims.x + u,
                                   (v + 1) * dims.x + (u + 1) + offset));

          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i(v * dims.x + u + offset,
                                   (v + 1) * dims.x + (u + 1) + offset,
                                   v * dims.x + u + 1 + offset,
                                   v * dims.x + u + 1));

          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i(v * dims.x + u + offset,
                                   (v + 1) * dims.x + (u + 1) + offset,
                                   (v + 1) * dims.x + u + offset,
                                   (v + 1) * dims.x + u));

          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i(v * dims.x + u + offset,
                                   (v + 1) * dims.x + (u + 1) + offset,
                                   (v + 1) * dims.x + u,
                                   v * dims.x + u + 1));
        }
      }

      tets_node->add(verts);
      tets_node->add(indices);

      auto vertexFields = std::make_shared<NodeList<DataVector1f>>();
      std::vector<sg::Any> vertexFieldNames;

      vertexFields->push_back(field);
      vertexFieldNames.push_back(std::string("GEN/VTX"));

      tets_node->add(vertexFields, "vertexFields");
      tets_node->createChild("vertexFieldName",
                             "string",
                             vertexFieldNames[0]).setWhiteList(vertexFieldNames);

      world->add(tets_node);
    }

    void generateWedges(const std::shared_ptr<Node> &world,
                        const std::vector<string_pair> &params)
    {
      auto tets_node = createNode("unstructured_wedges",
                                  "UnstructuredVolume");

      // get generator parameters

      vec2i dims(100, 100);

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 2) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxY'"
                      << std::endl;
            continue;
          }

          dims = vec2i(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()));
        } else {
          std::cout << "WARNING: unknown wedge generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate sphere data

      auto verts = createNode("vertices", "DataVector3f")->nodeAs<DataVector3f>();
      auto indices = createNode("indices", "DataVector4i")->nodeAs<DataVector4i>();
      auto field = createNode("field", "DataVector1f")->nodeAs<DataVector1f>();

      for (int w = 0; w < 2; w++) {
        for (int u = 0; u < dims.x; u++) {
          for (int v = 0; v < dims.y; v++) {
            float uu = M_PI * (-0.5f + u / float(dims.x - 1));
            float vv = M_PI * (v / float(dims.y - 1));
            float radius = 1.0f - float(w)/4;

            verts->push_back(radius * vec3f(std::cos(uu) * std::cos(vv),
                                            std::cos(uu) * std::sin(vv),
                                            std::sin(uu)));
            field->push_back(std::cos(4 * uu) * std::sin(4 *vv));
          }
        }
      }

      auto offset = dims.x * dims.y;
      for (int u = 0; u < dims.x - 1; u++) {
        for (int v = 0; v < dims.y - 1; v++) {
          indices->push_back(vec4i(-2, -2,
                                   v * dims.x + u,
                                   (v + 1) * dims.x + u));
          indices->push_back(vec4i(v * dims.x + (u + 1),
                                   v * dims.x + u + offset,
                                   (v + 1) * dims.x + u + offset,
                                   v * dims.x + (u + 1) + offset));

          indices->push_back(vec4i(-2, -2,
                                   (v + 1) * dims.x + u,
                                   (v + 1) * dims.x + (u + 1)));
          indices->push_back(vec4i(v * dims.x + (u + 1),
                                   (v + 1) * dims.x + u + offset,
                                   (v + 1) * dims.x + (u + 1) + offset,
                                   v * dims.x + (u + 1) + offset));
        }
      }

      tets_node->add(verts);
      tets_node->add(indices);

      auto vertexFields = std::make_shared<NodeList<DataVector1f>>();
      std::vector<sg::Any> vertexFieldNames;

      vertexFields->push_back(field);
      vertexFieldNames.push_back(std::string("GEN/VTX"));

      tets_node->add(vertexFields, "vertexFields");
      tets_node->createChild("vertexFieldName",
                             "string",
                             vertexFieldNames[0]).setWhiteList(vertexFieldNames);

      world->add(tets_node);
    }

    void generateHexahedrons(const std::shared_ptr<Node> &world,
                             const std::vector<string_pair> &params)
    {
      auto hex_node = createNode("unstructured_hexahedrons",
                                 "UnstructuredVolume");

      // get generator parameters

      vec3i dims(50, 50, 50);

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 3) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxY'"
                      << std::endl;
            continue;
          }

          dims = vec3i(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()),
                       std::atoi(string_dims[2].c_str()));
        } else {
          std::cout << "WARNING: unknown tetrahedron generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate sphere data

      auto verts = createNode("vertices", "DataVector3f")->nodeAs<DataVector3f>();
      auto indices = createNode("indices", "DataVector4i")->nodeAs<DataVector4i>();
      auto field = createNode("field", "DataVector1f")->nodeAs<DataVector1f>();

      for (int w = 0; w < dims.z; w++) {
        for (int u = 0; u < dims.x; u++) {
          for (int v = 0; v < dims.y; v++) {
            float uu = M_PI * (-0.5f + u / float(dims.x - 1));
            float vv = M_PI * (v / float(dims.y - 1));
            float ww = 0.5 + (w / float(dims.z - 1));

            verts->push_back(ww * vec3f(std::cos(uu) * std::cos(vv),
                                        std::cos(uu) * std::sin(vv),
                                        std::sin(uu)));
            field->push_back((ww - 0.5) * std::cos(4 * uu) * std::sin(4 *vv));
          }
        }
      }

      for (int u = 0; u < dims.x - 1; u++) {
        for (int v = 0; v < dims.y - 1; v++) {
          for (int w = 0; w < dims.z - 1; w++) {
            auto offset = vec4i(w * dims.x * dims.y);
            auto offsetNext = vec4i((w + 1) * dims.x * dims.y);
            auto loop = vec4i(v * dims.x + u,
                              v * dims.x + (u + 1),
                              (v + 1) * dims.x + (u + 1),
                              (v + 1) * dims.x + u);
            indices->push_back(offset + loop);
            indices->push_back(offsetNext + loop);
          }
        }
      }

      hex_node->add(verts);
      hex_node->add(indices);

      auto vertexFields = std::make_shared<NodeList<DataVector1f>>();
      std::vector<sg::Any> vertexFieldNames;

      vertexFields->push_back(field);
      vertexFieldNames.push_back(std::string("GEN/VTX"));

      hex_node->add(vertexFields, "vertexFields");
      hex_node->createChild("vertexFieldName",
                            "string",
                            vertexFieldNames[0]).setWhiteList(vertexFieldNames);

      world->add(hex_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateHexahedrons,  unstructuredHex  );
    OSPSG_REGISTER_GENERATE_FUNCTION(generateTetrahedrons, unstructuredTet  );
    OSPSG_REGISTER_GENERATE_FUNCTION(generateWedges,       unstructuredWedge);

  }  // ::ospray::sg
}  // ::ospray
