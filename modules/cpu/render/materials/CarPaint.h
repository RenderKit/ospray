// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "CarPaintShared.h"

namespace ospray {
namespace pathtracer {

struct CarPaint : public AddStructShared<Material, ispc::CarPaint>
{
  CarPaint();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
