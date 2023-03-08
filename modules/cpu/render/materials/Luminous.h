// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "LuminousShared.h"

namespace ospray {
namespace pathtracer {

struct Luminous : public AddStructShared<Material, ispc::Luminous>
{
  Luminous(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
