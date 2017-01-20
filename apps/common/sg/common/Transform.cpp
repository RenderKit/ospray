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

// sg components
#include "Transform.h"

namespace ospray {
  namespace sg {

    /*! \brief returns a std::string with the c++ name of this class */
    std::string Transform::toString() const
    { return "ospray::sg::Transform"; }
    
    /*! \brief 'render' the object for the first time */
    void Transform::render(RenderContext &ctx)
    {
      RenderContext newCtx(ctx,xfm);
      if (node) node->render(newCtx);
    }
    
    /*! \brief return bounding box in world coordinates.
      
      This function can be used by the viewer(s) for calibrating
      camera motion, setting default camera position, etc. Nodes
      for which that does not apply can simpy return
      box3f(empty) */
    box3f Transform::getBounds()
    {
      assert(node);
      const box3f nodeBounds = node->getBounds();
      const vec3f lo = nodeBounds.lower;
      const vec3f hi = nodeBounds.upper;
      box3f bounds = ospcommon::empty;
      bounds.extend(xfmPoint(xfm,vec3f(lo.x,lo.y,lo.z)));
      bounds.extend(xfmPoint(xfm,vec3f(hi.x,lo.y,lo.z)));
      bounds.extend(xfmPoint(xfm,vec3f(lo.x,hi.y,lo.z)));
      bounds.extend(xfmPoint(xfm,vec3f(hi.x,hi.y,lo.z)));
      bounds.extend(xfmPoint(xfm,vec3f(lo.x,lo.y,hi.z)));
      bounds.extend(xfmPoint(xfm,vec3f(hi.x,lo.y,hi.z)));
      bounds.extend(xfmPoint(xfm,vec3f(lo.x,hi.y,hi.z)));
      bounds.extend(xfmPoint(xfm,vec3f(hi.x,hi.y,hi.z)));
      return bounds;
    }

  } // ::ospray::sg
} // ::ospray
  
