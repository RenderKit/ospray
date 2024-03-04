// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "ThinGlassShared.h"

namespace ospray {
namespace pathtracer {

struct ThinGlass : public AddStructShared<Material, ispc::ThinGlass>
{
  ThinGlass(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;

 private:
  MaterialParam3f attenuationColor;
};

} // namespace pathtracer
} // namespace ospray
