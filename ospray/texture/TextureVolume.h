// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Texture.h"
#include "volume/VolumetricModel.h"

namespace ospray {

/*! \brief A Texture defined through a 2D Image. */
struct OSPRAY_SDK_INTERFACE TextureVolume : public Texture
{
  virtual ~TextureVolume() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  Ref<VolumetricModel> volume;
};

} // namespace ospray
