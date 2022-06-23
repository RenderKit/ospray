// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Future.h"

namespace ospray {

Future::Future()
{
  managedObjectType = OSP_FUTURE;
}

std::string Future::toString() const
{
  return "ospray::Future";
}

OSPTYPEFOR_DEFINITION(Future *);

} // namespace ospray
