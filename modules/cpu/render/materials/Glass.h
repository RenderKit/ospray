// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "GlassShared.h"

namespace ospray {
namespace pathtracer {

struct Glass : public AddStructShared<Material, ispc::Glass>
{
  Glass(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
