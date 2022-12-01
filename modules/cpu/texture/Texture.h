// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDeviceObject.h"
#include "common/ObjectFactory.h"
#include "common/StructShared.h"
// ispc shared
#include "TextureShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Texture
    : AddStructShared<ISPCDeviceObject, ispc::Texture>,
      public ObjectFactory<Texture, api::ISPCDevice &>
{
  Texture(api::ISPCDevice &device);
  virtual ~Texture() override = default;

  virtual std::string toString() const override;
};

OSPTYPEFOR_SPECIALIZATION(Texture *, OSP_TEXTURE);

} // namespace ospray
