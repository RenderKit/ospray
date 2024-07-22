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

  virtual bool isEmissive() const override;

 private:
  MaterialParam3f emissiveColor;
};

inline bool Luminous::isEmissive() const
{
  return reduce_max(getSh()->radiance) > 0.f;
}

} // namespace pathtracer
} // namespace ospray
