// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisMaterial.h"
// ispc
#include "render/scivis/SciVisMaterial_ispc.h"

namespace ospray {

SciVisMaterial::SciVisMaterial()
{
  ispcEquivalent = ispc::SciVisMaterial_create(this);
}

std::string SciVisMaterial::toString() const
{
  return "ospray::scivis::obj";
}

void SciVisMaterial::commit()
{
  MaterialParam1f d = getMaterialParam1f("d", 1.f);
  MaterialParam3f Kd = getMaterialParam3f("kd", vec3f(.8f));
  MaterialParam3f Ks = getMaterialParam3f("ks", vec3f(0.f));
  MaterialParam1f Ns = getMaterialParam1f("ns", 10.f);

  ispc::SciVisMaterial_set(getIE(),
      d.factor,
      d.tex,
      (const ispc::vec3f &)Kd.factor,
      Kd.tex,
      (const ispc::vec3f &)Ks.factor,
      Ks.tex,
      Ns.factor,
      Ns.tex);
}

} // namespace ospray
