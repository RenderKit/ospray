// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureVolume.h"
#include "TextureVolume_ispc.h"

#include "../common/Data.h"

namespace ospray {

std::string TextureVolume::toString() const
{
  return "ospray::TextureVolume";
}

void TextureVolume::commit()
{
  if (this->ispcEquivalent)
    ispc::delete_uniform(ispcEquivalent);

  auto *v = dynamic_cast<VolumetricModel *>(getParamObject("volume"));

  if (v == nullptr)
    throw std::runtime_error("volume texture must have 'volume' object");

  volume = v;

  this->ispcEquivalent = ispc::TextureVolume_create(v->getIE());
}

} // namespace ospray
