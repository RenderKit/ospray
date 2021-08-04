// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

namespace ospray {
namespace pathtracer {

struct MixMaterial : public ospray::Material
{
  MixMaterial();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
