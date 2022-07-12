// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
// ispc shared
#include "PlanesShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Planes
    : public AddStructShared<Geometry, ispc::Planes>
{
  Planes(api::ISPCDevice &device);
  virtual ~Planes() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  Ref<const DataT<vec4f>> coeffsData;
  Ref<const DataT<box3f>> boundsData;
};

} // namespace ospray
