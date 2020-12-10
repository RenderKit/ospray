// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include "texture/Texture2D_ispc.h"

#include "../common/Data.h"

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

  bool hascolorby = false;
  {
    const auto colormapping = getParam<std::string>("tex1dcolormapping", "");
    if (!colormapping.empty()) {
      EnsightTex1dMapping mapping(colormapping.c_str());
      this->map1d = mapping.d;
      hascolorby = true;
    }
  }

  bool hasalphaby = false;
  if (!hascolorby) {
    const auto alphamapping = getParam<std::string>("tex1dalphamapping", "");
    if (!alphamapping.empty()) {
      EnsightTex1dMapping mapping(alphamapping.c_str());
      this->map1d = mapping.d;
      hasalphaby = true;
    }
  }

  void *map1d = (hascolorby || hasalphaby) ? &this->map1d : nullptr;

  this->ispcEquivalent = ispc::Texture2D_create(
      (ispc::vec2i &)size, texData->data(), format, filter, map1d);
}

} // namespace ospray
