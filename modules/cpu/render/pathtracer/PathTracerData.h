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

  bool scannedForGeometryLights{false};

 private:
  ispc::Light *createGeometryLight(const Instance *instance,
      const GeometricModel *model,
      const int32 numPrimIDs,
      const std::vector<int> &primIDs,
      const std::vector<float> &distribution,
      float pdf);

  void generateGeometryLights(const World &world,
      const Renderer &renderer,
      std::vector<ispc::Light *> &lightShs);

  BufferSharedUq<ispc::Light *> lightArray;
  BufferSharedUq<float> lightCDFArray;

  std::vector<devicert::BufferShared<int>> geoLightPrimIDArray;
  std::vector<devicert::BufferShared<float>> geoLightDistrArray;
};

} // namespace ospray
