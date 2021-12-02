// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureVolume.h"
#include "texture/TextureVolume_ispc.h"

#include "common/Data.h"
#include "volume/transferFunction/TransferFunction.h"

namespace ospray {

std::string TextureVolume::toString() const
{
  return "ospray::TextureVolume";
}

TextureVolume::~TextureVolume()
{
  release();
}

void TextureVolume::release()
{
  if (ispcEquivalent) {
    ispc::TextureVolume_delete(ispcEquivalent);
    ispcEquivalent = nullptr;
  }
  volume = nullptr;
  transferFunction = nullptr;
  volumetricModel = nullptr;
}

void TextureVolume::commit()
{
  release();

  volume = dynamic_cast<Volume *>(getParamObject("volume"));
  if (volume) {
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);
    if (!transferFunction) {
      throw std::runtime_error(toString() + " must have 'transferFunction'");
    }
    ispcEquivalent =
        ispc::TextureVolume_create(volume->getIE(), transferFunction->getIE());
  } else {
    volumetricModel = dynamic_cast<VolumetricModel *>(getParamObject("volume"));
    if (!volumetricModel) {
      throw std::runtime_error(toString() + " must have 'volume' object");
    }
    ispcEquivalent =
        ispc::TextureVolume_create_deprecated(volumetricModel->getIE());
  }
}

} // namespace ospray
