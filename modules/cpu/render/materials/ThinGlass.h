// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

#include "ThinGlassShared.h"

namespace ospray {
namespace pathtracer {

struct ThinGlass : public AddStructShared<Material, ispc::ThinGlass>
{
  ThinGlass();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
