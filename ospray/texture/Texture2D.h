// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Texture.h"
#include "common/Data.h"

namespace ospray {

/*! \brief A Texture defined through a 2D Image. */
struct OSPRAY_SDK_INTERFACE Texture2D : public Texture
{
  virtual ~Texture2D() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  OSPTextureFormat format{OSP_TEXTURE_FORMAT_INVALID};
  OSPTextureFilter filter{OSP_TEXTURE_FILTER_BILINEAR};

 protected:
  Ref<const Data> texData;
};

} // namespace ospray
