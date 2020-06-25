// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Material.h"
#include "common/Util.h"
#include "texture/Texture2D.h"

#include "texture/TextureParam_ispc.h"

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

ispc::TextureParam Material::getTextureParam(const char *texture_name)
{
  // Get texture pointer
  Texture2D *ptr = (Texture2D *)getParamObject(texture_name);

  // Get 2D transformation if exists
  int transformFlags = ispc::TRANSFORM_FLAG_NONE;
  affine2f xfm2f = affine2f(one);
  utility::Optional<affine2f> xform2f = getTextureTransform2f(texture_name);
  if (xform2f.has_value()) {
    xfm2f = xform2f.value();
    transformFlags |= ispc::TRANSFORM_FLAG_2D;
  }

  // Get 3D transformation if exists
  affine3f xfm3f = affine3f(one);
  auto xform3f =
      getOptParam<affine3f>((std::string(texture_name) + ".transform").c_str());
  if (xform3f.has_value()) {
    xfm3f = xform3f.value();
    transformFlags |= ispc::TRANSFORM_FLAG_3D;
  }

  // Initialize ISPC structure
  ispc::TextureParam param;
  TextureParam_set(&param,
      ptr ? ptr->getIE() : nullptr,
      (ispc::TransformFlags)transformFlags,
      (const ispc::AffineSpace2f &)xfm2f,
      (const ispc::AffineSpace3f &)xfm3f);

  // Done
  return param;
}

MaterialParam1f Material::getMaterialParam1f(
    const char *name, float valIfNotFound)
{
  const std::string mapName = "map_" + std::string(name);
  MaterialParam1f param;
  param.tex = getTextureParam(mapName.c_str());
  param.rot = ((linear2f *)(&param.tex.xform2f.l))->orthogonal().transposed();
  param.factor = getParam<float>(name, param.tex.ptr ? 1.f : valIfNotFound);
  return param;
}

MaterialParam3f Material::getMaterialParam3f(
    const char *name, vec3f valIfNotFound)
{
  const std::string mapName = "map_" + std::string(name);
  MaterialParam3f param;
  param.tex = getTextureParam(mapName.c_str());
  param.rot = ((linear2f *)(&param.tex.xform2f.l))->orthogonal().transposed();
  param.factor =
      getParam<vec3f>(name, param.tex.ptr ? vec3f(1.f) : valIfNotFound);
  return param;
}

utility::Optional<affine2f> Material::getTextureTransform2f(
    const char *_texname)
{
  std::string texname(_texname);
  texname += ".";
  utility::Optional<affine2f> xform;

  // Apply translation
  const utility::Optional<vec2f> translation =
      getOptParam<vec2f>((texname + "translation").c_str());
  if (translation.has_value())
    xform = affine2f::translate(-translation.value());

  // Apply scale
  const utility::Optional<vec2f> scale =
      getOptParam<vec2f>((texname + "scale").c_str());
  if (scale.has_value()) {
    xform = xform.value_or(affine2f(one)) * affine2f::translate(vec2f(0.5f));
    xform = *xform * affine2f::scale(rcp(scale.value()));
    xform = *xform * affine2f::translate(vec2f(-0.5f));
  }

  // Apply rotation
  const utility::Optional<float> rotation =
      getOptParam<float>((texname + "rotation").c_str());
  if (rotation.has_value()) {
    xform = xform.value_or(affine2f(one)) * affine2f::translate(vec2f(0.5f));
    xform = *xform * affine2f::rotate(-deg2rad(rotation.value()));
    xform = *xform * affine2f::translate(vec2f(-0.5f));
  }

  // Apply complete transformation
  const utility::Optional<linear2f> transf =
      getOptParam<linear2f>((texname + "transform").c_str());
  if (transf.has_value())
    xform = xform.value_or(affine2f(one)) * affine2f(transf.value());
  const utility::Optional<vec4f> transf4 = // legacy / backwards compatible
      getOptParam<vec4f>((texname + "transform").c_str());
  if (transf4.has_value()) {
    const linear2f transform = (const linear2f &)transf4.value();
    xform = xform.value_or(affine2f(one)) * affine2f(transform);
  }

  // Return optional transformation
  return xform;
}

OSPTYPEFOR_DEFINITION(Material *);

} // namespace ospray
