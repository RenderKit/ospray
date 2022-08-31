// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "common/Data.h"
// ispc shared
#include "MeshShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Mesh : public AddStructShared<Geometry, ispc::Mesh>
{
  Mesh(api::ISPCDevice &device);
  virtual ~Mesh() override = default;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  Ref<const DataT<vec3f>> vertexData;
  Ref<const DataT<vec3f>> normalData;
  Ref<const Data> colorData;
  Ref<const DataT<vec2f>> texcoordData;
  Ref<const Data> indexData;
  bool motionBlur{false};
  Ref<const DataT<const DataT<vec3f> *>> motionVertexData;
  std::vector<vec3f *> motionVertexAddr;
  Ref<const DataT<const DataT<vec3f> *>> motionNormalData;
  std::vector<vec3f *> motionNormalAddr;
  range1f time{0.0f, 1.0f};
};

} // namespace ospray
