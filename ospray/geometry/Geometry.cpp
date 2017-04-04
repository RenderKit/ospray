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

// ospray 
#include "Geometry.h"
#include "common/Util.h"
// ISPC exports
#include "Geometry_ispc.h"

namespace ospray {

  Geometry::Geometry()
  {
    managedObjectType = OSP_GEOMETRY;
  }

  void Geometry::setMaterial(Material *mat)
  {
    if (!mat) {
      std::stringstream msg;
      msg << "#osp: warning - tried to set NULL material; ignoring"
          << "#osp: warning. (note this means that object may not get any "
          << "material at all!)" << std::endl;
      postErrorMsg(msg.str());
      return;
    }
    material = mat;
    if (!getIE()) 
      postErrorMsg("#osp: warning - geometry does not have an ispc equivalent!\n");
    else {
      ispc::Geometry_setMaterial(this->getIE(), mat ? mat->getIE() : nullptr);
    }
  }

  Material *Geometry::getMaterial() const
  {
    return material.ptr;
  }

  std::string Geometry::toString() const
  {
    return "ospray::Geometry";
  }

  void Geometry::finalize(Model *)
  {
  }

  Geometry *Geometry::createInstance(const char *type)
  {
    return createInstanceHelper<Geometry, OSP_GEOMETRY>(type);
  }

} // ::ospray
