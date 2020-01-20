// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.ih"

export void TransferFunction_set(
    void *uniform _self, const uniform range1f &range)
{
  TransferFunction *uniform self = (TransferFunction * uniform) _self;

  self->valueRange = range;
}
