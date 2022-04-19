// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "OSPCommon.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

export void delete_uniform(void *uniform uptr)
{
  delete uptr;
}

OSPRAY_END_ISPC_NAMESPACE
