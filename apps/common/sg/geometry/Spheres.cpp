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

#undef NDEBUG

#include "sg/geometry/Spheres.h"
#include "sg/common/Integrator.h"
// xml parser
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    Spheres::Sphere::Sphere(vec3f position, float radius, uint32_t typeID)
      : position(position), 
        radius(radius), 
        typeID(typeID) 
    {}

    box3f Spheres::bounds() const
    {
      box3f bounds = empty;
      for (auto &s : sphere)
        bounds.extend(s.bounds());
      return bounds;
    }

    //! \brief Initialize this node's value from given XML node 
    /*!
      \detailed This allows a plug-and-play concept where a XML
      file can specify all kind of nodes wihout needing to know
      their actual types: The XML parser only needs to be able to
      create a proper C++ instance of the given node type (the
      OSP_REGISTER_SG_NODE() macro will allow it to do so), and can
      tell the node to parse itself from the given XML content and
      XML children 
        
      \param node The XML node specifying this node's fields

      \param binBasePtr A pointer to an accompanying binary file (if
      existant) that contains additional binary data that the xml
      node fields may point into
    */
    void Spheres::setFromXML(const xml::Node &node,
                             const unsigned char *binBasePtr)
    {
      size_t num = std::stoll(node.getProp("num"));
      size_t ofs = std::stoll(node.getProp("ofs","-1"));
      float  rad = atof(node.getProp("radius").c_str());

      Spheres::Sphere s(vec3f(0.f),rad,0);
      if (ofs == (size_t)-1) {
        std::cout << "#osp:qtv: 'Spheres' ofs is '-1', "
                  << "generating set of random spheres..." << std::endl;
        for (uint32_t i = 0; i < num; i++) {
          s.position.x = drand48();
          s.position.y = drand48();
          s.position.z = drand48();
          s.radius = rad;
          sphere.push_back(s);
        }
      } else {
        const vec3f *in = (const vec3f*)(binBasePtr+ofs);
        for (uint32_t i = 0; i < num; i++) {
          memcpy(&s,&in[i],sizeof(*in));
          sphere.push_back(s);
        }
      }
    }

    void Spheres::render(RenderContext &ctx)
    {
      ospGeometry = ospNewGeometry("spheres");

      OSPData data = ospNewData(sphere.size()*5,OSP_FLOAT,
                                &sphere[0],OSP_DATA_SHARED_BUFFER);
      ospSetData(ospGeometry,"spheres",data);

      ospSet1i(ospGeometry,"bytes_per_sphere",sizeof(Spheres::Sphere));
      ospSet1i(ospGeometry,"center_offset",     0*sizeof(float));
      ospSet1i(ospGeometry,"offset_radius",     3*sizeof(float));
      ospSet1i(ospGeometry,"offset_materialID", 4*sizeof(float));

      auto mat = ospNewMaterial(ctx.integrator->handle(), "default");
      if (mat) {
        vec3f kd = vec3f(.7f);
        ospSet3fv(mat,"kd",&kd.x);
        ospCommit(mat);
      }
      ospSetMaterial(ospGeometry,mat);
      ospCommit(ospGeometry);
      
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
      ospCommit(data);
    }

    struct RandomSpheres : public Spheres
    {
      //! \brief Initialize this node's value from given XML node 
      void setFromXML(const xml::Node *const node, 
                      const unsigned char *binBasePtr)
      {
        vec3i dimensions = toVec3i(node->getProp("dimensions").c_str());
        int   num        = toInt(node->getProp("num").c_str());
        
        float max_r = atof(node->getProp("radius").c_str());
        float f = 0.3f; // overhang around the dimensions
        for (int i = 0; i < num; i++) {
          vec3f pos; 
          pos.x = (-f+(1+2*f)*drand48())*dimensions.x;
          pos.y = (-f+(1+2*f)*drand48())*dimensions.y;
          pos.z = (-f+(1+2*f)*drand48())*dimensions.z;
          float r = max_r*(0.5f + 0.5f*drand48());
          sphere.push_back(Sphere(pos,r));
        }
      }
    };

    OSP_REGISTER_SG_NODE(RandomSpheres);
    OSP_REGISTER_SG_NODE(Spheres);
  }
}
