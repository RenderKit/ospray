// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "CarPaintShared.h"

namespace ospray {
namespace pathtracer {

struct CarPaint : public AddStructShared<Material, ispc::CarPaint>
{
  CarPaint(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;

 private:
  MaterialParam3f baseColor;
  MaterialParam1f roughness;
  MaterialParam1f normal;
  MaterialParam3f flakeColor;
  MaterialParam1f flakeScale;
  MaterialParam1f flakeDensity;
  MaterialParam1f flakeSpread;
  MaterialParam1f flakeJitter;
  MaterialParam1f flakeRoughness;
  MaterialParam1f coat;
  MaterialParam1f coatIor;
  MaterialParam3f coatColor;
  MaterialParam1f coatThickness;
  MaterialParam1f coatRoughness;
  MaterialParam1f coatNormal;
  MaterialParam3f flipflopColor;
  MaterialParam1f flipflopFalloff;
};

} // namespace pathtracer
} // namespace ospray
