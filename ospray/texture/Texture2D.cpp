// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include "Texture2D_ispc.h"

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

  this->ispcEquivalent = ispc::Texture2D_create(
      (ispc::vec2i &)size, texData->data(), format, filter);
}

} // namespace ospray
