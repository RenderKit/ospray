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
#include "Material.h"
#include "common/Library.h"
// stl
#include <map>

namespace ospray {

  typedef Material *(*creatorFct)();

  std::map<std::string, creatorFct> materialRegistry;


  /*! \brief creates an abstract material class of given type 
    
    The respective material type must be a registered material type
    in either ospray proper or any already loaded module. For
    material types specified in special modules, make sure to call
    ospLoadModule first. */
  Material *Material::createMaterial(const char *_type)
  {
    std::string type(_type);
    for (size_t i = 0; i < type.size(); i++)
      if (type[i] == '-') type[i] = '_';

    std::map<std::string, Material *(*)()>::iterator it = materialRegistry.find(type);
    if (it != materialRegistry.end())
      return it->second ? (it->second)() : NULL;
    
    if (ospray::logLevel >= 2) 
      std::cout << "#ospray: trying to look up material type '" 
                << type << "' for the first time..." << std::endl;

    std::string creatorName = "ospray_create_material__"+type;
    creatorFct creator = (creatorFct)getSymbol(creatorName);
    materialRegistry[type] = creator;
    if (creator == NULL) {
      if (ospray::logLevel >= 1) 
        std::cout << "#ospray: could not find material type '" << type << "'" << std::endl;
      return NULL;
    }
    Material *material = (*creator)();
    material->managedObjectType = OSP_MATERIAL;
    return material;
  }

  affine2f Material::getTextureTransform(const char* _texname)
  {
    std::string texname(_texname);
    texname += ".";

    const vec2f translation = getParam2f((texname+"translation").c_str(), vec2f(0.f));
    affine2f xform = affine2f::translate(-translation);

    xform *= affine2f::translate(vec2f(0.5f));

    const vec2f scale = getParam2f((texname+"scale").c_str(), vec2f(1.f));
    xform *= affine2f::scale(rcp(scale));

    const float rotation = deg2rad(getParam1f((texname+"rotation").c_str(), 0.f));
    xform *= affine2f::rotate(-rotation);

    xform *= affine2f::translate(vec2f(-0.5f));

    const vec4f transf = getParam4f((texname+"transform").c_str(), vec4f(1.f, 0.f, 0.f, 1.f));
    const linear2f transform = (linear2f&)transf;
    xform *= affine2f(transform);

    return xform;
  }


} // ::ospray
