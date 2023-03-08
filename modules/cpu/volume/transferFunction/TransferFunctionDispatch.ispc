// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.ih"
#include "TransferFunctionDispatch.ih"
#include "TransferFunctionShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f TransferFunction_dispatch_get(
    const TransferFunction *uniform self, float value)
{
  switch (self->type) {
  case TRANSFER_FUNCTION_TYPE_LINEAR:
    return LinearTransferFunction_get(self, value);
  default:
    return make_vec4f(0.f);
  }
}

SYCL_EXTERNAL float TransferFunction_dispatch_getMaxOpacity(
    const TransferFunction *uniform self, const range1f &valueRange)
{
  switch (self->type) {
  case TRANSFER_FUNCTION_TYPE_LINEAR:
    return LinearTransferFunction_getMaxOpacity(self, valueRange);
  default:
    return 0.f;
  }
}

OSPRAY_END_ISPC_NAMESPACE
