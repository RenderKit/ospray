// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "PlasticShared.h"

namespace ospray {
namespace pathtracer {

struct Plastic : public AddStructShared<Material, ispc::Plastic>
{
  Plastic();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
