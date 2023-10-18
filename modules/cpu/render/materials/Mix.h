// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "MixShared.h"

namespace ospray {
namespace pathtracer {

struct MixMaterial : public AddStructShared<Material, ispc::Mix>
{
  MixMaterial(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;

 protected:
  Ref<Material> mat1;
  Ref<Material> mat2;
};

} // namespace pathtracer
} // namespace ospray
