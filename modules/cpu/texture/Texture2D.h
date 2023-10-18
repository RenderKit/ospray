// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Texture.h"
#include "common/Data.h"
// ispc shared
#include "Texture2DShared.h"

namespace ospray {

// A Texture defined through a 2D Image
struct OSPRAY_SDK_INTERFACE Texture2D
    : public AddStructShared<Texture, ispc::Texture2D>
{
  Texture2D(api::ISPCDevice &device)
      : AddStructShared(device.getIspcrtContext(), device)
  {}
  virtual ~Texture2D() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  OSPTextureFormat format{OSP_TEXTURE_FORMAT_INVALID};
  OSPTextureFilter filter{OSP_TEXTURE_FILTER_LINEAR};

 protected:
  Ref<const Data> texData;
};

} // namespace ospray
