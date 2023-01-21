// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Material.h"
#include "render/bsdfs/MicrofacetAlbedoTables.h"
#include "texture/Texture.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc
#include "render/Material_ispc.h"
#endif

namespace ospray {

// Material definitions ///////////////////////////////////////////////////////

Ref<MicrofacetAlbedoTables> Material::microfacetAlbedoTables = nullptr;

Material::Material(api::ISPCDevice &device, const FeatureFlagsOther ffo)
    : AddStructShared(device.getIspcrtContext(), device), featureFlags(ffo)
{
  managedObjectType = OSP_MATERIAL;
#ifndef OSPRAY_TARGET_SYCL
  getSh()->getTransparency =
      reinterpret_cast<ispc::Material_GetTransparencyFunc>(
          ispc::Material_getTransparency_addr());
  getSh()->selectNextMedium =
      reinterpret_cast<ispc::Material_SelectNextMediumFunc>(
          ispc::Material_selectNextMedium_addr());
#endif

  if (!microfacetAlbedoTables) {
    microfacetAlbedoTables = new MicrofacetAlbedoTables(device);
    // Release the extra local ref
    microfacetAlbedoTables->refDec();
  } else {
    microfacetAlbedoTables->refInc();
  }

  getSh()->microfacetAlbedoTables = microfacetAlbedoTables->getSh();
}

Material::~Material()
{
  if (microfacetAlbedoTables) {
    const bool lastReference = microfacetAlbedoTables->useCount() == 1;
    // The last material referencing the albedo tables should null out the
    // pointer so we don't try to call refDec again and know to re-create it
    // when a new material is made
    if (lastReference) {
      microfacetAlbedoTables = nullptr;
    } else {
      microfacetAlbedoTables->refDec();
    }
  }
}

std::string Material::toString() const
{
  return "ospray::Material";
}

void Material::commit() {}

ispc::TextureParam Material::getTextureParam(const char *texture_name)
{
  // Get texture pointer
  Texture *ptr = (Texture *)getParamObject(texture_name);
  if (ptr)
    featureFlags |= FFO_TEXTURE_IN_MATERIAL;

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
  param.ptr = ptr ? ptr->getSh() : nullptr;
  param.transformFlags = (ispc::TransformFlags)transformFlags;
  param.xform2f = xfm2f;
  param.xform3f = xfm3f;

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
