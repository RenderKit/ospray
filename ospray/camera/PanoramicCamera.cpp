// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanoramicCamera.h"
#include "PanoramicCamera_ispc.h"

namespace ospray {

PanoramicCamera::PanoramicCamera()
{
  ispcEquivalent = ispc::PanoramicCamera_create(this);
}

std::string PanoramicCamera::toString() const
{
  return "ospray::PanoramicCamera";
}

} // namespace ospray
