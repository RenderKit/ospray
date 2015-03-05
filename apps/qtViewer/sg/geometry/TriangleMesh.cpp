// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include "sg/geometry/TriangleMesh.h"
#include "sg/common/World.h"
#include "sg/common/Integrator.h"

namespace ospray {
  namespace sg {

    //! return the bounding box of all primitives
    box3f TriangleMesh::getBounds()
    {
      box3f bounds = empty;
      for (int i=0;i<vertex->getSize();i++)
        bounds.extend(vertex->get3f(i));
      return bounds;
    }

    //! return the bounding box of all primitives
    box3f PTMTriangleMesh::getBounds()
    {
      box3f bounds = empty;
      for (int i=0;i<vertex->getSize();i++)
        bounds.extend(vertex->get3f(i));
      return bounds;
    }

    /*! 'render' the nodes */
    void TriangleMesh::render(RenderContext &ctx)
    {
      if (ospGeometry) return;

      assert(ctx.world);
      assert(ctx.world->ospModel);

      ospGeometry = ospNewTriangleMesh();
      // set vertex data
      if (vertex->notEmpty())
        ospSetData(ospGeometry,"vertex",vertex->getOSP());
      if (normal->notEmpty())
        ospSetData(ospGeometry,"vertex.normal",normal->getOSP());
      if (texcoord->notEmpty())
        ospSetData(ospGeometry,"vertex.texcoord",texcoord->getOSP());
      // set triangle data
      if (triangle->notEmpty())
        ospSetData(ospGeometry,"triangle",triangle->getOSP());

      // assign a default material (for now.... eventually we might
      // want to do a 'real' material
      OSPMaterial mat = ospNewMaterial(ctx.integrator?ctx.integrator->getOSPHandle():NULL,"default");
      if (mat) {
        vec3f kd = .7f;
        vec3f ks = .3f;
        ospSet3fv(mat,"kd",&kd.x);
        ospSet3fv(mat,"ks",&ks.x);
        ospSet1f(mat,"Ns",99.f);
        ospCommit(mat);
      }
      ospSetMaterial(ospGeometry,mat);

      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    /*! 'render' the nodes */
    void PTMTriangleMesh::render(RenderContext &ctx)
    {
      PRINT(this);
      if (ospGeometry) return;

      assert(ctx.world);
      assert(ctx.world->ospModel);

      ospGeometry = ospNewTriangleMesh();
      // set vertex arrays
      if (vertex && vertex->notEmpty())
        ospSetData(ospGeometry,"vertex",vertex->getOSP());
      if (normal && normal->notEmpty())
        ospSetData(ospGeometry,"vertex.normal",normal->getOSP());
      if (texcoord && texcoord->notEmpty())
        ospSetData(ospGeometry,"vertex.texcoord",texcoord->getOSP());
      // set triangle array
      if (triangle && triangle->notEmpty())
        ospSetData(ospGeometry,"triangle",triangle->getOSP());
      
      // assign a default material (for now.... eventually we might
      // want to do a 'real' material
      OSPMaterial mat = ospNewMaterial(ctx.integrator?ctx.integrator->getOSPHandle():NULL,"default");
      if (mat) {
        vec3f kd = .7f;
        vec3f ks = .3f;
        ospSet3fv(mat,"kd",&kd.x);
        ospSet3fv(mat,"ks",&ks.x);
        ospSet1f(mat,"Ns",99.f);
        ospCommit(mat);
      }
      ospSetMaterial(ospGeometry,mat);

      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

  } // ::ospray::sg
} // ::ospray


