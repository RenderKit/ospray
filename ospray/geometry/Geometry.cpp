// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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
#include "common/Library.h"
// stl 
#include <map>
// ISPC exports
#include "Geometry_ispc.h"

namespace ospray {

  typedef Geometry *(*creatorFct)();

  std::map<std::string, creatorFct> geometryRegistry;

  //! set given geometry's material. 
  /*! all material assignations should go through this function; the
    'material' field itself is private). This allows the
    respective geometry's derived instance to always properly set
    the material field of the ISCP-equivalent whenever the
    c++-side's material gets changed */
  void Geometry::setMaterial(Material *mat)
  {
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
    std::map<std::string, Geometry *(*)()>::iterator it = geometryRegistry.find(type);
    if (it != geometryRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up geometry type '" 
                << type << "' for the first time" << std::endl;

    std::string creatorName = "ospray_create_geometry__"+std::string(type);
    creatorFct creator = (creatorFct)getSymbol(creatorName);
    geometryRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find geometry type '" << type << "'" << std::endl;
      return NULL;
    }
    Geometry *geometry = (*creator)();  geometry->managedObjectType = OSP_GEOMETRY;
    return(geometry);
  }

} // ::ospray
