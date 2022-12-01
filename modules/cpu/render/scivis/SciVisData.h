// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/StructShared.h"
#include "render/RendererData.h"
// ispc shared
#include "SciVisDataShared.h"

namespace ospray {

struct World;

struct SciVisData : public AddStructShared<RendererData, ispc::SciVisData>
{
  SciVisData(const World &world);
  ~SciVisData() override;

 private:
  std::vector<ISPCRTMemoryView> lightViews;
  std::unique_ptr<BufferShared<ispc::Light *>> lightArray;
};

} // namespace ospray
