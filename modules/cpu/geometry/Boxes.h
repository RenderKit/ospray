// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
// ispc shared
#include "BoxesShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Boxes
    : public AddStructShared<Geometry, ispc::Boxes>
{
  Boxes(api::ISPCDevice &device);
  virtual ~Boxes() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  Ref<const DataT<box3f>> boxData;
};

} // namespace ospray
