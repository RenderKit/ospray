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

  virtual bool isEmissive() const override;

 private:
  MaterialParam3f baseColor;
  MaterialParam3f edgeColor;
  MaterialParam1f metallic;
  MaterialParam1f diffuse;
  MaterialParam1f specular;
  MaterialParam1f ior;
  MaterialParam1f transmission;
  MaterialParam3f transmissionColor;
  MaterialParam1f transmissionDepth;
  MaterialParam1f roughness;
  MaterialParam1f anisotropy;
  MaterialParam1f rotation;
  MaterialParam1f normal;
  MaterialParam1f baseNormal;
  MaterialParam1f coat;
  MaterialParam1f coatIor;
  MaterialParam3f coatColor;
  MaterialParam1f coatThickness;
  MaterialParam1f coatRoughness;
  MaterialParam1f coatNormal;
  MaterialParam1f sheen;
  MaterialParam3f sheenColor;
  MaterialParam1f sheenTint;
  MaterialParam1f sheenRoughness;
  MaterialParam1f opacity;
  MaterialParam1f backlight;
  MaterialParam1f thickness;
  MaterialParam3f emissiveColor;
};

inline bool Principled::isEmissive() const
{
  return reduce_max(getSh()->emission) > 0.f;
}

} // namespace pathtracer
} // namespace ospray
