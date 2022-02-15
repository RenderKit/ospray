// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "MixShared.h"

namespace ospray {
namespace pathtracer {

struct MixMaterial : public AddStructShared<Material, ispc::Mix>
{
  MixMaterial();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
