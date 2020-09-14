// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Texture.h"
#include "volume/Volume.h"
#include "volume/VolumetricModel.h"
#include "volume/transferFunction/TransferFunction.h"

namespace ospray {

/*! \brief A Texture defined through a 2D Image. */
struct OSPRAY_SDK_INTERFACE TextureVolume : public Texture
{
  virtual ~TextureVolume() override;

  virtual std::string toString() const override;

  virtual void commit() override;

 private:
  void release();
  Ref<Volume> volume;
  Ref<TransferFunction> transferFunction;

  // Deprecated interface.
  Ref<VolumetricModel> volumetricModel;
};

} // namespace ospray
