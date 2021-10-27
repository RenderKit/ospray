// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Texture.h"
#include "common/StructShared.h"
#include "volume/Volume.h"
#include "volume/VolumetricModel.h"
#include "volume/transferFunction/TransferFunction.h"

#include "TextureVolumeShared.h"

namespace ospray {

// A Texture defined through a volume
struct OSPRAY_SDK_INTERFACE TextureVolume
    : public AddStructShared<Texture, ispc::TextureVolume>
{
  virtual std::string toString() const override;

  virtual void commit() override;

 private:
  Ref<Volume> volume;
  Ref<TransferFunction> transferFunction;

  // Deprecated interface.
  Ref<VolumetricModel> volumetricModel;
};

} // namespace ospray
