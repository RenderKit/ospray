// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "PrincipledShared.h"

namespace ospray {
namespace pathtracer {

struct Principled : public AddStructShared<Material, ispc::Principled>
{
  Principled(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
