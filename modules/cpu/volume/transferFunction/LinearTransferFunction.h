// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray
#include "TransferFunction.h"
#include "common/Data.h"
// ispc shared
#include "LinearTransferFunctionShared.h"

namespace ospray {

// piecewise linear transfer function
struct OSPRAY_SDK_INTERFACE LinearTransferFunction
    : public AddStructShared<TransferFunction, ispc::LinearTransferFunction>
{
  LinearTransferFunction(api::ISPCDevice &device);

  virtual void commit() override;

  virtual std::string toString() const override;

  virtual std::vector<range1f> getPositiveOpacityValueRanges() const override;
  virtual std::vector<range1i> getPositiveOpacityIndexRanges() const override;

 private:
  Ref<const DataT<vec3f>> colorValues;
  Ref<const DataT<float>> opacityValues;

  void precomputeMaxOpacityRanges();
};

} // namespace ospray
