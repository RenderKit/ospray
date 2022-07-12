// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/spectrum.h"
#include "render/Material.h"
// ispc shared
#include "MetallicPaintShared.h"

namespace ospray {
namespace pathtracer {

struct MetallicPaint : public AddStructShared<Material, ispc::MetallicPaint>
{
  MetallicPaint(api::ISPCDevice &device);

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace pathtracer
} // namespace ospray
