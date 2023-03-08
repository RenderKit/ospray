// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDeviceObject.h"
#include "common/ObjectFactory.h"
#include "common/StructShared.h"
// ispc shared
#include "TransferFunctionShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE TransferFunction
    : public AddStructShared<ISPCDeviceObject, ispc::TransferFunction>,
      public ObjectFactory<TransferFunction, api::ISPCDevice &>
{
  TransferFunction(api::ISPCDevice &device);
  virtual void commit() override;
  virtual std::string toString() const override;

  range1f valueRange;
  virtual std::vector<range1f> getPositiveOpacityValueRanges() const = 0;
  virtual std::vector<range1i> getPositiveOpacityIndexRanges() const = 0;
};

OSPTYPEFOR_SPECIALIZATION(TransferFunction *, OSP_TRANSFER_FUNCTION);

} // namespace ospray
