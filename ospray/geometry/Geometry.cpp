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
#include "common/Library.h"
// stl 
#include <map>
// ISPC exports
#include "Geometry_ispc.h"

namespace ospray {

  //! set given geometry's material. 
  /*! all material assignations should go through this function; the
    'material' field itself is private). This allows the
    respective geometry's derived instance to always properly set
    the material field of the ISCP-equivalent whenever the
    c++-side's material gets changed */
  void Geometry::setMaterial(Material *mat)
  {
    if (!mat) {
      std::cout << "#osp: warning - tried to set NULL material; ignoring"
        "#osp: warning. (note this means that object may not get any material at all!)" << std::endl;
      return;
    }
    material = mat;
    if (!getIE()) 
      std::cout << "#osp: warning - geometry does not have an ispc equivalent!" << std::endl;
    else {
      ispc::Geometry_setMaterial(this->getIE(),mat?mat->getIE():NULL);
    }
  }


  /*! \brief creates an abstract geometry class of given type 
    
    The respective geometry type must be a registered geometry type
    in either ospray proper or any already loaded module. For
    geometry types specified in special modules, make sure to call
    ospLoadModule first. */
  Geometry *Geometry::createGeometry(const char *type)
  {
    return createInstanceHelper<Geometry, OSP_GEOMETRY>(type);
  }

} // ::ospray
