// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "AlloyShared.h"

namespace ospray {
namespace pathtracer {

struct Alloy : public AddStructShared<Material, ispc::Alloy>
{
  Alloy();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
