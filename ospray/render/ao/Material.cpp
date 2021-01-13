// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.h"
// ispc
#include "render/ao/Material_ispc.h"

namespace ospray {

AOMaterial::AOMaterial()
{
  ispcEquivalent = ispc::AOMaterial_create(this);
}

void AOMaterial::commit()
{
  MaterialParam1f d = getMaterialParam1f("d", 1.f);
  MaterialParam3f Kd = getMaterialParam3f("kd", vec3f(.8f));

  ispc::AOMaterial_set(
      getIE(), d.factor, d.tex, (const ispc::vec3f &)Kd.factor, Kd.tex);
}

} // namespace ospray
