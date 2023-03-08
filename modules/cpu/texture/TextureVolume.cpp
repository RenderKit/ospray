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
  volume = dynamic_cast<Volume *>(getParamObject("volume"));
  if (volume) {
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);
    if (!transferFunction) {
      throw std::runtime_error(toString() + " must have 'transferFunction'");
    }
    getSh()->volume = volume->getSh();
    getSh()->transferFunction = transferFunction->getSh();
  } else {
    volumetricModel = dynamic_cast<VolumetricModel *>(getParamObject("volume"));
    if (!volumetricModel) {
      throw std::runtime_error(toString() + " must have 'volume' object");
    }
    getSh()->volume = volumetricModel->getSh()->volume;
    getSh()->transferFunction = volumetricModel->getSh()->transferFunction;
  }
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.get =
      reinterpret_cast<ispc::Texture_get>(ispc::TextureVolume_get_addr());
  getSh()->super.getNormal =
      reinterpret_cast<ispc::Texture_getN>(ispc::TextureVolume_getN_addr());
#endif
}

} // namespace ospray
#endif
