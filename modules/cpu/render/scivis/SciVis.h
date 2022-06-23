// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "../Renderer.h"

namespace ospray {

struct SciVis : public Renderer
{
  SciVis();
  std::string toString() const override;
  void commit() override;
  void *beginFrame(FrameBuffer *, World *) override;
};

} // namespace ospray
