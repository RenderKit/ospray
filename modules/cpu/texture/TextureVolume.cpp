// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifdef OSPRAY_ENABLE_VOLUMES

#include "TextureVolume.h"
#ifndef OSPRAY_TARGET_SYCL
#include "texture/TextureVolume_ispc.h"
#endif

namespace ospray {

std::string TextureVolume::toString() const
{
  return "ospray::TextureVolume";
}

void TextureVolume::commit()
{
  volume = getParamObject<Volume>("volume");
  if (!volume)
    throw std::runtime_error(toString() + " must have 'volume' object");

  transferFunction = getParamObject<TransferFunction>("transferFunction");
  if (!transferFunction)
    throw std::runtime_error(toString() + " must have 'transferFunction'");

  getSh()->volume = volume->getSh();
  getSh()->transferFunction = transferFunction->getSh();

#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.get =
      reinterpret_cast<ispc::Texture_get>(ispc::TextureVolume_get_addr());
#endif
}

} // namespace ospray
#endif
