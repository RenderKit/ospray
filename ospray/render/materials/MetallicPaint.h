// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "render/Material.h"

namespace ospray {
namespace pathtracer {

struct MetallicPaint : public ospray::Material
{
  MetallicPaint();

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
