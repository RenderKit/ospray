// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "PrincipledShared.h"

namespace ospray {
namespace pathtracer {

struct Principled : public AddStructShared<Material, ispc::Principled>
{
  Principled();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
