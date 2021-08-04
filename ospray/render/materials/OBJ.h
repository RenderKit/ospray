// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

namespace ospray {
namespace pathtracer {

struct OBJMaterial : public ospray::Material
{
  OBJMaterial();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
