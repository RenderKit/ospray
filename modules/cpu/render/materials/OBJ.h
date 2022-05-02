// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"
// ispc shared
#include "OBJShared.h"

namespace ospray {
namespace pathtracer {

struct OBJMaterial : public AddStructShared<Material, ispc::OBJ>
{
  OBJMaterial();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
