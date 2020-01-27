// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "common/Material.h"
#include "texture/Texture2D.h"
// ispc
#include "SciVisMaterial_ispc.h"

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

// SciVisMaterial definitions /////////////////////////////////////////////

SciVisMaterial::SciVisMaterial()
{
  ispcEquivalent = ispc::SciVisMaterial_create(this);
}

void SciVisMaterial::commit()
{
  Kd = getParam<vec3f>("kd", vec3f(.8f));
  d = getParam<float>("d", 1.f);
  map_Kd = (Texture2D *)getParamObject("map_kd");
  ispc::SciVisMaterial_set(
      getIE(), (const ispc::vec3f &)Kd, d, map_Kd ? map_Kd->getIE() : nullptr);
}

OSP_REGISTER_MATERIAL(scivis, SciVisMaterial, obj);

// NOTE(jda) - support all renderer aliases
OSP_REGISTER_MATERIAL(ao, SciVisMaterial, obj);

} // namespace ospray
