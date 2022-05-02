// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "render/Renderer.h"
// ispc shared
#include "SciVisShared.h"

namespace ospray {

struct SciVis : public AddStructShared<Renderer, ispc::SciVis>
{
  SciVis();
  std::string toString() const override;
  void commit() override;
  void *beginFrame(FrameBuffer *, World *) override;
};

} // namespace ospray
