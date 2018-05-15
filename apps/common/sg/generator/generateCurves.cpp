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
#include "Generator.h"
#include <random>
#include <ospcommon/vec.h>

namespace ospray {
  namespace sg {
    using namespace ospcommon;

    struct ControlPoint {
      vec3f pos;
      float rad;
    };
    struct BezierCurve {
      ControlPoint cp[4];
    };

    float gaussian()
    {
      static std::default_random_engine r;
      static std::normal_distribution<float> g(0,1);
      return g(r);
    }

    BezierCurve generateOne(float length, float width, float variance)
    {
      BezierCurve curve;

      float base_radius = width * (1.f+0.1f*variance*gaussian());
      curve.cp[0].rad = base_radius;
      curve.cp[1].rad = 2.f/3.f * base_radius * fabsf(1.f+.1f*variance*gaussian());
      curve.cp[2].rad = 1.f/3.f * base_radius * fabsf(1.f+.2f*variance*gaussian());
      curve.cp[3].rad = 0.f;
      

      float height = length*(1+0.1f*variance*gaussian());
      curve.cp[0].pos
        = vec3f(drand48(),drand48(),0.f);
      curve.cp[3].pos
        = curve.cp[0].pos
        + vec3f(0,0,height)
        + 0.1f*height*variance*vec3f(gaussian(),gaussian(),gaussian());

      vec3f C
        = 0.5f*(curve.cp[0].pos+curve.cp[3].pos) 
        + 0.3f*height*variance*vec3f(gaussian(),gaussian(),0.4f*gaussian());

      curve.cp[1].pos
        = (1.f*C + 2.f*curve.cp[0].pos)/3.f;
      curve.cp[2].pos
        = (1.f*C + 2.f*curve.cp[3].pos)/3.f;

      return curve;
    }

    void generateCurves(const std::shared_ptr<Node> &world,
                           const std::vector<string_pair> &// params
                        )
    {
      size_t numCurves = 100;
      float  length    = 1;
      float  width     = .3f;
      float  variance  = 1.f;

      std::vector<BezierCurve> bezierCurves;
      for (size_t i=0;i<numCurves;i++)
        bezierCurves.push_back(generateOne(length*1.f/sqrtf(numCurves),width*1.f/sqrtf(numCurves),variance));



      auto curves_node = createNode("pile_of_curves", "StreamLines");
     
      auto curves_vertices = std::make_shared<DataVector3fa>();
      curves_vertices->setName("vertex");
      auto curves_indices = std::make_shared<DataVector1i>();
      curves_indices->setName("index");
      for (auto bc : bezierCurves) {
        curves_indices->v.push_back(curves_vertices->v.size());
        curves_indices->v.push_back(curves_vertices->v.size()+1);
        curves_indices->v.push_back(curves_vertices->v.size()+2);
        curves_vertices->v.push_back((const vec3fa&)bc.cp[0]);
        curves_vertices->v.push_back((const vec3fa&)bc.cp[1]);
        curves_vertices->v.push_back((const vec3fa&)bc.cp[2]);
        curves_vertices->v.push_back((const vec3fa&)bc.cp[3]);
      }

      curves_node->createChild("smooth", "bool", false);
      curves_node->add(curves_vertices);
      curves_node->add(curves_indices);
      
      world->add(curves_node);
    }
    
    OSPSG_REGISTER_GENERATE_FUNCTION(generateCurves, curves);
    
  }  // ::ospray::sg
}  // ::ospray
