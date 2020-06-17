// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SciVisMaterial.h"
// ispc
#include "SciVisMaterial_ispc.h"

namespace ospray {

SciVisMaterial::SciVisMaterial()
{
  ispcEquivalent = ispc::SciVisMaterial_create(this);
}

void SciVisMaterial::commit()
{
  MaterialParam1f d = getMaterialParam1f("d", 1.f);
  MaterialParam3f Kd = getMaterialParam3f("kd", vec3f(.8f));

  ispc::SciVisMaterial_set(
      getIE(), d.factor, d.tex, (const ispc::vec3f &)Kd.factor, Kd.tex);
}

} // namespace ospray
