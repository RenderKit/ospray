// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Material.h"
#include "texture/Texture2D.h"

namespace ospray {

struct SciVisMaterial : public ospray::Material
{
  SciVisMaterial();
  void commit() override;

 private:
  vec3f Kd;
  float d;
  Ref<Texture2D> map_Kd;
};

} // namespace ospray
