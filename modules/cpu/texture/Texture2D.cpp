// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include "common/OSPCommon_ispc.h"
#include "texture/Texture2D_ispc.h"

#include "../common/Data.h"

namespace ispc {

void Texture2D::Set(const vec2i &aSize,
    void *aData,
    const EnsightTex1dMappingData &aMap1d,
    OSPTextureFormat type,
    OSPTextureFilter flags)
{
  size = aSize;

  // Due to float rounding frac(x) can be exactly 1.0f (e.g. for very small
  // negative x), although it should be strictly smaller than 1.0f. We handle
  // this case by having sizef slightly smaller than size, such that
  // frac(x)*sizef is always < size.
  sizef =
      vec2f(nextafter((float)size.x, -1.0f), nextafter((float)size.y, -1.0f));
  halfTexel = vec2f(0.5f / size.x, 0.5f / size.y);
  data = aData;
  map1d = aMap1d;

  super.get =
      ispc::Texture2D_get_addr(type, flags & OSP_TEXTURE_FILTER_NEAREST);
  super.getNormal =
      ispc::Texture2D_getN_addr(type, flags & OSP_TEXTURE_FILTER_NEAREST);
  super.hasAlpha = type == OSP_TEXTURE_RGBA8 || type == OSP_TEXTURE_SRGBA
      || type == OSP_TEXTURE_RA8 || type == OSP_TEXTURE_LA8
      || type == OSP_TEXTURE_RGBA32F || type == OSP_TEXTURE_RGBA16
      || type == OSP_TEXTURE_RA16;
}

} // namespace ispc

namespace ospray {

std::string Texture2D::toString() const
{
  return "ospray::Texture2D";
}

void Texture2D::commit()
{
  texData = getParam<Data *>("data");

  if (!texData || texData->numItems.z > 1) {
    throw std::runtime_error(toString()
        + " must have 2D 'data' array using the first two dimensions.");
  }

  const vec2i size = vec2i(texData->numItems.x, texData->numItems.y);
  if (!texData->compact()) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " does currently not support strides, copying texture data.";

    auto data = new Data(texData->type, texData->numItems);
    data->copy(*texData, vec3ui(0));
    texData = data;
    data->refDec();
  }

  format = static_cast<OSPTextureFormat>(
      getParam<int>("format", OSP_TEXTURE_FORMAT_INVALID));
  filter = static_cast<OSPTextureFilter>(
      getParam<int>("filter", OSP_TEXTURE_FILTER_BILINEAR));

  if (format == OSP_TEXTURE_FORMAT_INVALID)
    throw std::runtime_error(toString() + ": invalid 'format'");

  if (sizeOf(format) != sizeOf(texData->type))
    throw std::runtime_error(toString() + ": 'format'='" + stringFor(format)
        + "' does not match type of 'data'='" + stringFor(texData->type)
        + "'!");

  EnsightTex1dMappingData map1d;
  bool hascolorby = false;
  {
    const auto strmapping = getParam<std::string>("tex1dcolormapping", "");
    if (!strmapping.empty()) {
      EnsightTex1dMapping mapping(strmapping.c_str());
      map1d = mapping.d;
      hascolorby = true;
    }
  }

  //we only allow one mapping approach--colorby or alphaby, not both
  if (!hascolorby) {
    const auto strmapping = getParam<std::string>("tex1dalphamapping", "");
    if (!strmapping.empty()) {
      EnsightTex1dMapping mapping(strmapping.c_str());
      map1d = mapping.d;
    }
  }

  getSh()->Set(size, texData->data(), map1d, format, filter);
}

} // namespace ospray
