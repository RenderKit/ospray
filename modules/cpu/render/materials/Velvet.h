// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "VelvetShared.h"

namespace ospray {
namespace pathtracer {

struct Velvet : public AddStructShared<Material, ispc::Velvet>
{
  Velvet(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
