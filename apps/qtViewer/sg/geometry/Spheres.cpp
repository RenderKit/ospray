// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "apps/common/xml/XML.h"

namespace ospray {
  namespace sg {

    using std::string;
    using std::cout;
    using std::endl;

    Spheres::Sphere::Sphere(vec3f position, 
                            float radius,
                            uint typeID) 
      : position(position), 
        radius(radius), 
        typeID(typeID) 
    {}

    box3f Spheres::getBounds() 
    {
      box3f bounds = embree::empty;
      for (size_t i=0;i<sphere.size();i++)
        bounds.extend(sphere[i].getBounds());
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
    void Spheres::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr) 
    {
      size_t num = node->getPropl("num");
      size_t ofs = node->getPropl("ofs");
      float  rad = atof(node->getProp("radius").c_str());
      PRINT(rad);

      Spheres::Sphere s(vec3f(0.f),rad,0);
      if (ofs == (size_t)-1) {
        cout << "#osp:qtv: 'Spheres' ofs is '-1', generating set of random spheres..." << endl;
        for (int i=0;i<num;i++) {
          s.position.x = drand48();
          s.position.y = drand48();
          s.position.z = drand48();
          s.radius = rad;
          sphere.push_back(s);
        }
      } else {
        for (int i=0;i<num;i++) {
          const vec3f *in = (const vec3f*)(binBasePtr+ofs);
          memcpy(&s,&in[i],sizeof(s));
          sphere.push_back(s);
        }
      }
    }


    void Spheres::render(RenderContext &ctx)
    {
      assert(!ospGeometry);

      ospGeometry = ospNewGeometry("spheres");
      assert(ospGeometry);

      OSPData data = ospNewData(sphere.size()*5,OSP_FLOAT,
                                &sphere[0],OSP_DATA_SHARED_BUFFER);
      ospSetData(ospGeometry,"spheres",data);

      // ospSet1f(geom,"radius",radius*particleModel[i]->radius);
      ospSet1i(ospGeometry,"bytes_per_sphere",sizeof(Spheres::Sphere));
      ospSet1i(ospGeometry,"center_offset",     0*sizeof(float));
      ospSet1i(ospGeometry,"offset_radius",     3*sizeof(float));
      ospSet1i(ospGeometry,"offset_materialID", 4*sizeof(float));
      // ospSetData(geom,"materialList",materialData);

      OSPMaterial mat = ospNewMaterial(ctx.integrator?ctx.integrator->getOSPHandle():NULL,"default");
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
 
    OSP_REGISTER_SG_NODE(Spheres);
    
  }
}
