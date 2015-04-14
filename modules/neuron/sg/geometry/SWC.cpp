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

/*! \file importer/SVC.cpp Scene Graph Importer Plugin for Neuromorpho SWC files */

// ospray
// ospray/sg
#include "SWC.h"
#include "sg/common/Integrator.h"
	 
namespace ospray {
  namespace sg {
    
    SWCGeometry::SWCGeometry() 
      : Geometry("SWC"), ospNodeData(NULL), ospLinkData(NULL), ospGeometry(NULL), checkData(true)
    {}
      
    //! return bounding box of all primitives
    box3f SWCGeometry::getBounds()
    {
      box3f bounds = embree::empty;
      for (int i=0;i<nodeVec.size();i++) {
        bounds.extend(nodeVec[i].pos-vec3f(nodeVec[i].rad));
        bounds.extend(nodeVec[i].pos+vec3f(nodeVec[i].rad));
      }
      return bounds;
    }

    void SWCGeometry::render(RenderContext &ctx)
    {
      if (ospGeometry) 
        return;

      ospLoadModule("neuron");
      ospGeometry = ospNewGeometry("neuron");
      
      ospLinkData = ospNewData(linkVec.size(),OSP_UINT2,&linkVec[0],
                               OSP_DATA_SHARED_BUFFER);
      ospSetData(ospGeometry,"linkData",ospLinkData);
      ospNodeData = ospNewData(nodeVec.size(),OSP_FLOAT4,&nodeVec[0],
                               OSP_DATA_SHARED_BUFFER);
      PRINT((vec4f&)nodeVec[0]);
      ospSetData(ospGeometry,"nodeData",ospNodeData);

      ospSet1i(ospGeometry,"checkData",checkData);

      ospCommit(ospGeometry);

      // assign a default material (for now.... eventaully we might
      // want to do a 'real' mateiral
      OSPMaterial mat = ospNewMaterial(ctx.integrator?ctx.integrator->getOSPHandle():NULL,"default");
      if (mat) {
        vec3f kd(.7,.2,.2);
        vec3f ks = .3f;
        ospSet3fv(mat,"kd",&kd.x);
        ospSet3fv(mat,"ks",&ks.x);
        ospSet1f(mat,"Ns",99.f);
        ospCommit(mat);
      }
      ospSetMaterial(ospGeometry,mat);
      
      // and finally, add this geometry to the model
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
      
    }
    
  }
}

