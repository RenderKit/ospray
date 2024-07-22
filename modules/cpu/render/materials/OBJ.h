// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "OBJShared.h"

namespace ospray {
namespace pathtracer {

struct OBJMaterial : public AddStructShared<Material, ispc::OBJ>
{
  OBJMaterial(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;

 private:
  Ref<Texture> ref_bumpTex;

  MaterialParam1f d;
  MaterialParam3f Kd;
  MaterialParam3f Ks;
  MaterialParam1f Ns;
};

} // namespace pathtracer
} // namespace ospray
