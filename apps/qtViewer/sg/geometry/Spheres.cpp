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
        vec3f kd = .7f;
        ospSet3fv(mat,"kd",&kd.x);
        ospCommit(mat);
      }
      ospSetMaterial(ospGeometry,mat);
      ospCommit(ospGeometry);
      
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
      ospCommit(data);
    }

  }
}
