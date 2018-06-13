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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow-uncaptured-local"

// ospcommon
#include "ospcommon/containers/AlignedVector.h"
// sg
#include "SceneGraph.h"
#include "sg/common/Material.h"
#include "sg/geometry/TriangleMesh.h"
#include "sg/geometry/StreamLines.h"

namespace ospray {
  namespace sg {
    namespace osx {

      // Helper types /////////////////////////////////////////////////////////

      struct Triangles
      {
        containers::AlignedVector<vec3fa> vertex;
        containers::AlignedVector<vec4f>  color; // vertex color, from sv's 'v' value
        containers::AlignedVector<vec3i>  index;

        vec3f lerpf(float x, float x0, float x1, vec3f y0, vec3f y1)
        {
          float f = (x-x0)/(x1-x0);
          return f*y1+(1-f)*y0;
        }

        vec4f colorOf(const float f)
        {
          if (f < .5f)
            return vec4f(lerpf(f, 0.f,.5f,vec3f(0),vec3f(0,1,0)), 1.f);
          else
            return vec4f(lerpf(f, .5f,1.f,vec3f(0,1,0),vec3f(1,0,0)), 1.f);
        }
      };

      struct StreamLines
      {
        containers::AlignedVector<vec3fa> vertex;
        containers::AlignedVector<int>    index;
        float radius {0.001f};
      };

      static const char *delim = "\n\t\r ";

      void osxParseInts(containers::AlignedVector<int> &vec, const std::string &content)
      {
        char *s = strdup(content.c_str());
        char *tok = strtok(s,delim);
        while (tok) {
          vec.push_back(atol(tok));
          tok = strtok(NULL,delim);
        }
        free(s);
      }

      template<typename T> T ato(const char *);
      template<> inline int ato<int>(const char *s) { return atol(s); }
      template<> inline float ato<float>(const char *s) { return atof(s); }

      template<typename T>
      vec_t<T,3> osxParseVec3(char * &tok) {
        vec_t<T,3> v;

        assert(tok);
        v.x = ato<T>(tok);
        tok = strtok(NULL,delim);

        assert(tok);
        v.y = ato<T>(tok);
        tok = strtok(NULL,delim);

        assert(tok);
        v.z = ato<T>(tok);
        tok = strtok(NULL,delim);

        return v;
      }

      void osxParseVec3is(containers::AlignedVector<vec3i> &vec, const std::string &content)
      {
        char *s = strdup(content.c_str());
        char *tok = strtok(s,delim);
        while (tok)
          vec.push_back(osxParseVec3<int>(tok));
        free(s);
      }

      void osxParseVec3fas(containers::AlignedVector<vec3fa> &vec, const std::string &content)
      {
        char *s = strdup(content.c_str());
        char *tok = strtok(s,delim);
        while (tok)
          vec.push_back(osxParseVec3<float>(tok));
        free(s);
      }

      void osxParseColors(containers::AlignedVector<vec4f> &vec, const std::string &content)
      {
        char *s = strdup(content.c_str());
        char *tok = strtok(s,delim);
        while (tok)
          vec.push_back(vec4f(osxParseVec3<float>(tok), 1.f));
        free(s);
      }

      /*! parse ospray xml file */
      void parseOSX(StreamLines *streamLines,
                    Triangles *triangles,
                    const std::string &fn)
      {
        std::shared_ptr<xml::XMLDoc> doc = xml::readXML(fn);
        assert(doc);
        if (doc->child.size() != 1 || doc->child[0].name != "OSPRay")
          throw std::runtime_error("could not parse osx file: Not in OSPRay format!?");
        const xml::Node &root_element = doc->child[0];
        for (const auto &node : root_element.child) {
          if (node.name == "Model") {
            const xml::Node &model_node = node;
            for (const auto &node : model_node.child) {
              if (node.name == "StreamLines") {
                const xml::Node &sl_node = node;
                for (const auto &node : sl_node.child) {
                  if (node.name == "vertex")
                    osxParseVec3fas(streamLines->vertex, node.content);
                  else if (node.name == "index")
                    osxParseInts(streamLines->index, node.content);
                }
              } else if (node.name == "TriangleMesh") {
                const xml::Node &tris_node = node;
                for (const auto &node : tris_node.child) {
                  if (node.name == "vertex")
                    osxParseVec3fas(triangles->vertex, node.content);
                  else if (node.name == "color")
                    osxParseColors(triangles->color, node.content);
                  else if (node.name == "index")
                    osxParseVec3is(triangles->index, node.content);
                }
              }
            }
          }
        }
      }
    }

    void importOSX(const std::shared_ptr<Node> &world,
                   const FileName &fileName)
    {
      osx::StreamLines streamLines;
      osx::Triangles   triangles;
      std::cout << "parsing OSX input file... \n";
      parseOSX(&streamLines, &triangles, fileName);
      std::cout << "...finished parsing...\n";

      auto name = fileName.str();

      if (!streamLines.index.empty()) {
        std::cout << "...adding found streamlines to the scene...\n";
        auto slNode = createNode(name + "_streamlines",
                                 "StreamLines")->nodeAs<StreamLines>();

        auto v  = createNode("vertex","DataVector3fa")->nodeAs<DataVector3fa>();
        auto vi = createNode("index", "DataVector1i")->nodeAs<DataVector1i>();

        v->v  = std::move(streamLines.vertex);
        vi->v = std::move(streamLines.index);

        slNode->add(v);
        slNode->add(vi);

        slNode->child("radius") = streamLines.radius;

        auto model = createNode(name + "_streamlines_model", "Model");
        model->add(slNode);

        auto instance = createNode(name + "_streamlines_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        auto matNodePtr =
          slNode->createChild("material", "Material").nodeAs<Material>();
        auto &matNode = *matNodePtr;

        matNode["type"] = std::string("default");

        matNode["d"]  = 1.f;
        matNode["Kd"] = vec3f(0.9f, 0.9f, 0.9f);
        matNode["Ks"] = vec3f(0.2f, 0.2f, 0.2f);

        world->add(instance);
      }

      if (!triangles.index.empty()) {
        std::cout << "...adding found triangles to the scene...\n";
        auto slNode = createNode(name + "_triangles",
                                 "TriangleMesh")->nodeAs<StreamLines>();
        slNode->remove("material");

        auto v  = createNode("vertex","DataVector3fa")->nodeAs<DataVector3fa>();
        auto vi = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
        auto vc = createNode("color", "DataVector4f")->nodeAs<DataVector4f>();

        v->v  = std::move(triangles.vertex);
        vi->v = std::move(triangles.index);
        vc->v = std::move(triangles.color);

        slNode->add(v);
        slNode->add(vi);
        if(!vc->empty()) slNode->add(vc);

        auto model = createNode(name + "_triangles_model", "Model");
        model->add(slNode);

        auto instance = createNode(name + "_triangles_instance", "Instance");
        instance->setChild("model", model);
        model->setParent(instance);

        world->add(instance);
      }

      std::cout << "...finished import!\n";
    }

  } // ::ospray::sg
} // ::ospray

#pragma clang diagnostic pop
