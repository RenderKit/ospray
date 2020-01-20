// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "TransferFunction.h"
#include "TransferFunction_ispc.h"
#include "common/Util.h"

namespace ospray {

TransferFunction::TransferFunction()
{
  managedObjectType = OSP_TRANSFER_FUNCTION;
}

void TransferFunction::commit()
{
  auto param = getParam<vec2f>("valueRange", vec2f(0.0f, 1.0f));
  valueRange = range1f(param.x, param.y);
  ispc::TransferFunction_set(ispcEquivalent, (const ispc::box1f &)(valueRange));
}

std::string TransferFunction::toString() const
{
  return "ospray::TransferFunction";
}

TransferFunction *TransferFunction::createInstance(const std::string &type)
{
  return createInstanceHelper<TransferFunction, OSP_TRANSFER_FUNCTION>(type);
}

OSPTYPEFOR_DEFINITION(TransferFunction *);

} // namespace ospray
