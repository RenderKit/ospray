// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "CarPaintShared.h"

namespace ospray {
namespace pathtracer {

struct CarPaint : public AddStructShared<Material, ispc::CarPaint>
{
  CarPaint(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
