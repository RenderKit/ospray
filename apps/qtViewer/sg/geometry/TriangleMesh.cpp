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
      // set index data
      if (index->notEmpty())
        ospSetData(ospGeometry,"index",index->getOSP());

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
      //std::cout << "#qtViewer 'rendered' mesh \n";
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
      // set index array
      if (index && index->notEmpty())
        ospSetData(ospGeometry,"index",index->getOSP());
      
      Triangle *triangles = (Triangle*)index->getBase();
      for(size_t i = 0; i < index->getSize(); i++) {
        materialIDs.push_back(triangles[i].materialID >> 16);
      }
      primMatIDs = ospNewData(materialIDs.size(), OSP_INT, &materialIDs[0], 0);
      ospSetData(ospGeometry,"prim.materialID",primMatIDs);
      
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
      
      std::vector<OSPMaterial> ospMaterials;
      for (size_t i = 0; i < materialList.size(); i++) {
        assert(materialList[i] != NULL);
        //If the material hasn't already been 'rendered' ensure that it is.
        materialList[i]->render(ctx);
        //Push the 'rendered' material onto the list
        ospMaterials.push_back(materialList[i]->ospMaterial);
        //std::cout << "#qtViewer 'rendered' material " << materialList[i]->name << std::endl;
      }
      
      OSPData materialData = ospNewData(materialList.size(), OSP_OBJECT, &ospMaterials[0], 0);
      ospSetData(ospGeometry, "materialList", materialData);

      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
      //std::cout << "#qtViewer 'rendered' mesh\n";
    }

  } // ::ospray::sg
} // ::ospray


