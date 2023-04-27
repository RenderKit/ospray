// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#ifndef OSPRAY_TARGET_SYCL
#include "texture/Texture2D_ispc.h"
#endif
#include "texture/MipMapGeneration_ispc.h"

#include "../common/Data.h"

namespace ispc {

void Texture2D::set(const rkcommon::math::vec2i &aSize,
    void **aData,
    int aMaxLevel,
    OSPTextureFormat aFormat,
    OSPTextureFilter aFilter,
    const rkcommon::math::vec2ui &aWrapMode,
    const rkcommon::math::vec4f &aAvg)
{
  size = aSize;
  maxLevel = aMaxLevel;
  format = aFormat;
  filter = aFilter;
  wrapMode = aWrapMode;
  avg = aAvg;

  for (int l = 0; l <= aMaxLevel; l++)
    data[l] = aData[l];

  super.hasAlpha = aFormat == OSP_TEXTURE_RGBA8 || aFormat == OSP_TEXTURE_SRGBA
      || aFormat == OSP_TEXTURE_RA8 || aFormat == OSP_TEXTURE_LA8
      || aFormat == OSP_TEXTURE_RGBA32F || aFormat == OSP_TEXTURE_RGBA16
      || aFormat == OSP_TEXTURE_RA16 || aFormat == OSP_TEXTURE_RA16F
      || aFormat == OSP_TEXTURE_RA32F || aFormat == OSP_TEXTURE_RGBA16F;
#ifndef OSPRAY_TARGET_SYCL
  super.get = reinterpret_cast<ispc::Texture_get>(
      ispc::Texture2D_get_addr(aFormat, aFilter & OSP_TEXTURE_FILTER_NEAREST));
  super.getNormal = reinterpret_cast<ispc::Texture_getN>(
      ispc::Texture2D_getN_addr(aFormat, aFilter & OSP_TEXTURE_FILTER_NEAREST));
#endif
}

} // namespace ispc

namespace ospray {

std::string Texture2D::toString() const
{
  return "ospray::Texture2D";
}

void Texture2D::commit()
{
  texData = getParamObject<Data>("data");

  if (!texData || texData->numItems.z > 1) {
    throw std::runtime_error(toString()
        + " must have 2D 'data' array using the first two dimensions.");
  }

  if (texData->size() > std::numeric_limits<std::uint32_t>::max())
    throw std::runtime_error(toString()
        + " too large (over 4B texels, indexing is limited to 32bit");

  const vec2i size = vec2i(texData->numItems.x, texData->numItems.y);
  if (!texData->compact()) {
    postStatusMsg(OSP_LOG_INFO)
        << toString()
        << " does currently not support strides, copying texture data.";

    auto data = new Data(getISPCDevice(), texData->type, texData->numItems);
    data->copy(*texData, vec3ui(0));
    texData = data;
    data->refDec();
  }

  format = static_cast<OSPTextureFormat>(
      getParam<uint32_t>("format", OSP_TEXTURE_FORMAT_INVALID));
  filter = static_cast<OSPTextureFilter>(
      getParam<uint32_t>("filter", OSP_TEXTURE_FILTER_LINEAR));
  wrapMode = getParam<vec2ui>("wrapMode",
      vec2ui(getParam<uint32_t>("wrapMode", OSP_TEXTURE_WRAP_REPEAT)));

  if (format == OSP_TEXTURE_FORMAT_INVALID)
    throw std::runtime_error(toString() + ": invalid 'format'");

  if (sizeOf(format) != sizeOf(texData->type))
    throw std::runtime_error(toString() + ": 'format'='" + stringFor(format)
        + "' does not match type of 'data'='" + stringFor(texData->type)
        + "'!");

  std::vector<void *> dataPtr;
  dataPtr.emplace_back(texData->data());
  vec4f avg{0.f, 0.f, 0.f, 1.f};

  if (getISPCDevice().disableMipMapGeneration)
    mipMapData = nullptr;
  else {
    // conservatively estimate needed space for MIP maps and allocate
    const auto maxMipTexels = (size.product() + 2) / 3 // for square textures
        + (size.x + size.y - 1) / std::min(size.x, size.y); // remaining 1D case
    mipMapData = devicert::make_buffer_shared_unique<char>(
        getISPCDevice().getDRTDevice(), maxMipTexels * sizeOf(format));

    // generate MIP map levels
    vec2i prevSize = size;
    void *prevData = texData->data();
    char *dstData = (char *)mipMapData->data();
    while (prevSize != vec2i(1)) {
      assert(dataPtr.size() <= MAX_MIPMAP_LEVEL); // max tex size is 2^32
      vec2i dstSize(max(prevSize.x / 2, 1), max(prevSize.y / 2, 1));
      ispc::MipMap_generate(prevData,
          (ispc::vec2i &)prevSize,
          dstData,
          (ispc::vec2i &)dstSize,
          (ispc::vec4f &)avg,
          format);

      dataPtr.emplace_back(dstData);
      prevSize = dstSize;
      prevData = dstData;
      dstData += dstSize.product() * ospray::sizeOf(format);
    }
  }

  // Initialize ispc shared structure
  getSh()->set(
      size, dataPtr.data(), dataPtr.size() - 1, format, filter, wrapMode, avg);
}

} // namespace ospray
