// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Material.h"

namespace ospray {

struct AOMaterial : public ospray::Material
{
  AOMaterial();
  void commit() override;
};

} // namespace ospray
