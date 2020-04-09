// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Material.h"
#include "common/Util.h"

namespace ospray {

static FactoryMap<Material> g_materialsMap;

// Material definitions ///////////////////////////////////////////////////////

Material::Material()
{
  managedObjectType = OSP_MATERIAL;
}

Material *Material::createInstance(
    const char *_renderer_type, const char *_material_type)
{
  std::string renderer_type = _renderer_type;
  std::string material_type = _material_type;

  std::string name = renderer_type + "_" + material_type;
  return createInstanceHelper(name, g_materialsMap[name]);
}

void Material::registerType(const char *name, FactoryFcn<Material> f)
{
  g_materialsMap[name] = f;
}

std::string Material::toString() const
{
  return "ospray::Material";
}

void Material::commit() {}

affine2f Material::getTextureTransform(const char *_texname)
{
  std::string texname(_texname);
  texname += ".";

  const vec2f translation =
      getParam<vec2f>((texname + "translation").c_str(), vec2f(0.f));
  affine2f xform = affine2f::translate(-translation);

  xform *= affine2f::translate(vec2f(0.5f));

  const vec2f scale = getParam<vec2f>((texname + "scale").c_str(), vec2f(1.f));
  xform *= affine2f::scale(rcp(scale));

  const float rotation =
      deg2rad(getParam<float>((texname + "rotation").c_str(), 0.f));
  xform *= affine2f::rotate(-rotation);

  xform *= affine2f::translate(vec2f(-0.5f));

  const vec4f transf = getParam<vec4f>(
      (texname + "transform").c_str(), vec4f(1.f, 0.f, 0.f, 1.f));
  const linear2f transform = (const linear2f &)transf;
  xform *= affine2f(transform);

  return xform;
}

MaterialParam1f Material::getMaterialParam1f(
    const char *name, float valIfNotFound)
{
  const std::string mapName = "map_" + std::string(name);
  MaterialParam1f param;
  param.map = (Texture2D *)getParamObject(mapName.c_str());
  param.xform = getTextureTransform(mapName.c_str());
  param.rot = param.xform.l.orthogonal().transposed();
  param.factor = getParam<float>(name, param.map ? 1.f : valIfNotFound);
  return param;
}

MaterialParam3f Material::getMaterialParam3f(
    const char *name, vec3f valIfNotFound)
{
  const std::string mapName = "map_" + std::string(name);
  MaterialParam3f param;
  param.map = (Texture2D *)getParamObject(mapName.c_str());
  param.xform = getTextureTransform(mapName.c_str());
  param.rot = param.xform.l.orthogonal().transposed();
  param.factor = getParam<vec3f>(name, param.map ? vec3f(1.f) : valIfNotFound);
  return param;
}

OSPTYPEFOR_DEFINITION(Material *);

} // namespace ospray
