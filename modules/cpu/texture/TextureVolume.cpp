// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureVolume.h"
#include "texture/TextureVolume_ispc.h"

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
  getSh()->super.get = ispc::TextureVolume_get_addr();
  getSh()->super.getNormal = ispc::TextureVolume_getN_addr();
}

} // namespace ospray
