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
      // set vertex array
      if (vertex->notEmpty())
        ospSetData(ospGeometry,"vertex",vertex->getOSP());
      // set triangle array
      if (triangle->notEmpty())
        ospSetData(ospGeometry,"triangle",triangle->getOSP());
      
      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    /*! 'render' the nodes */
    void PTMTriangleMesh::render(RenderContext &ctx)
    {
      if (ospGeometry) return;

      assert(ctx.world);
      assert(ctx.world->ospModel);

      ospGeometry = ospNewTriangleMesh();
      // set vertex array
      if (vertex->notEmpty())
        ospSetData(ospGeometry,"vertex",vertex->getOSP());
      // set triangle array
      if (triangle->notEmpty())
        ospSetData(ospGeometry,"triangle",triangle->getOSP());
      
      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

  } // ::ospray::sg
} // ::ospray


