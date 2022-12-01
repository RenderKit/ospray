// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/StructShared.h"
#include "render/RendererData.h"
// ispc shared
#include "PathTracerDataShared.h"

namespace ospray {

struct World;
struct Instance;
struct GeometricModel;
struct Renderer;

struct PathTracerData
    : public AddStructShared<RendererData, ispc::PathTracerData>
{
  PathTracerData(const World &world,
      bool importanceSampleGeometryLights,
      const Renderer &renderer);
  ~PathTracerData() override;

 private:
  ISPCRTMemoryView createGeometryLight(const Instance *instance,
      const GeometricModel *model,
      const std::vector<int> &primIDs,
      const std::vector<float> &distribution,
      float pdf);

  void generateGeometryLights(const World &world, const Renderer &renderer);

  std::vector<ISPCRTMemoryView> lightViews;
  std::unique_ptr<BufferShared<ispc::Light *>> lightArray;
  std::unique_ptr<BufferShared<float>> lightCDFArray;

  std::vector<BufferShared<int>> geoLightPrimIDArray;
  std::vector<BufferShared<float>> geoLightDistrArray;
};

} // namespace ospray
