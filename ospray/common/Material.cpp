// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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
#include "common/Util.h"

namespace ospray {

  // Helper functions /////////////////////////////////////////////////////////

  static Material* tryToCreateMaterial(const std::string &renderer_type,
                                       const std::string &material_type)
  {
    std::string type = std::string(renderer_type) + "__" + material_type;
    Material *mat = nullptr;
    try {
      mat = createInstanceHelper<Material, OSP_MATERIAL>(type.c_str());
    } catch (const std::runtime_error &) {
      // ignore...
    }
    return mat;
  }

  // Material definitions /////////////////////////////////////////////////////

  Material *Material::createInstance(const char *_renderer_type,
                                     const char *_material_type)
  {
    std::string renderer_type = _renderer_type;
    std::string material_type = _material_type;

    // Try to create the given material
    auto mat = tryToCreateMaterial(renderer_type, material_type);
    if (mat != nullptr)
      return mat;

    // Looks like we failed to create the given type, try default as a backup
    mat = tryToCreateMaterial(renderer_type, "default");
    if (mat != nullptr)
      return mat;

    // The renderer doesn't even provide a default material, provide a generic
    // one (that it will ignore) so API calls still work for the application
    if (material_type == "OBJMaterial" ||
        material_type == "SciVisMaterial" ||
        material_type == "default") {
      return new Material;
    } else {
      return nullptr;
    }
  }

  std::string Material::toString() const
  {
    return "ospray::Material";
  }

  void Material::commit()
  {
  }

  affine2f Material::getTextureTransform(const char* _texname)
  {
    std::string texname(_texname);
    texname += ".";

    const vec2f translation = getParam2f((texname+"translation").c_str(),
                                         vec2f(0.f));
    affine2f xform = affine2f::translate(-translation);

    xform *= affine2f::translate(vec2f(0.5f));

    const vec2f scale = getParam2f((texname+"scale").c_str(), vec2f(1.f));
    xform *= affine2f::scale(rcp(scale));

    const float rotation = deg2rad(getParam1f((texname+"rotation").c_str(),
                                              0.f));
    xform *= affine2f::rotate(-rotation);

    xform *= affine2f::translate(vec2f(-0.5f));

    const vec4f transf = getParam4f((texname+"transform").c_str(),
                                    vec4f(1.f, 0.f, 0.f, 1.f));
    const linear2f transform = (const linear2f&)transf;
    xform *= affine2f(transform);

    return xform;
  }

  MaterialParam1f Material::getMaterialParam1f(const char *name, float valIfNotFound)
  {
    const std::string mapName = std::string(name) + "Map";
    MaterialParam1f param;
    param.map = (Texture2D*)getParamObject(mapName.c_str());
    param.xform = getTextureTransform(mapName.c_str());
    param.rot = param.xform.l.orthogonal().transposed();
    param.factor = getParam1f(name, param.map ? 1.f : valIfNotFound);
    return param;
  }

  MaterialParam3f Material::getMaterialParam3f(const char *name, vec3f valIfNotFound)
  {
    const std::string mapName = std::string(name) + "Map";
    MaterialParam3f param;
    param.map = (Texture2D*)getParamObject(mapName.c_str());
    param.xform = getTextureTransform(mapName.c_str());
    param.rot = param.xform.l.orthogonal().transposed();
    param.factor = getParam3f(name, param.map ? vec3f(1.f) : valIfNotFound);
    return param;
  }

} // ::ospray
