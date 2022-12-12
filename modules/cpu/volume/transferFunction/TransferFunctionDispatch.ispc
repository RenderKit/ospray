// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LinearTransferFunction.ih"
#include "TransferFunctionDispatch.ih"
#include "TransferFunctionShared.h"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL vec4f TransferFunction_dispatch_get(
    const TransferFunction *uniform self, float value)
{
  return LinearTransferFunction_get(self, value);
  // OSPRay only has one transfer function type, so just call it right now.
  // If we add more this can become a switch
}

SYCL_EXTERNAL float TransferFunction_dispatch_getMaxOpacity(
    const TransferFunction *uniform self, const range1f &valueRange)
{
  // OSPRay only has one transfer function type, so just call it right now.
  // If we add more this can become a switch
  return LinearTransferFunction_getMaxOpacity(self, valueRange);
}

OSPRAY_END_ISPC_NAMESPACE

