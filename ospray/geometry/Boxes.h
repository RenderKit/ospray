// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "volume/Volume.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Boxes : public Geometry
{
  Boxes();
  virtual ~Boxes() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  Ref<const DataT<box3f>> boxData;
};

} // namespace ospray
