// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "VelvetShared.h"

namespace ospray {
namespace pathtracer {

struct Velvet : public AddStructShared<Material, ispc::Velvet>
{
  Velvet();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
