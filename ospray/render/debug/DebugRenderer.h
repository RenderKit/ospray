// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "render/Renderer.h"

namespace ospray {

struct DebugRenderer : public Renderer
{
  DebugRenderer();
  virtual ~DebugRenderer() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;
};

} // namespace ospray
