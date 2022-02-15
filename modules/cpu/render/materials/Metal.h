// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/spectrum.h"
#include "render/Material.h"

#include "MetalShared.h"

namespace ospray {
namespace pathtracer {

struct Metal : public AddStructShared<Material, ispc::Metal>
{
  Metal();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
