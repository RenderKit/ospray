// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Geometry.h"
#include "volume/VolumetricModel.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Isosurfaces : public Geometry
{
  Isosurfaces();
  virtual ~Isosurfaces() override;

  virtual std::string toString() const override;

  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  // Data members //

  Ref<const DataT<float>> isovaluesData;
  Ref<VolumetricModel> model;
  VKLValueSelector valueSelector{nullptr};
};

} // namespace ospray
