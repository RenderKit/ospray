// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Instance.ih"

export void *uniform Instance_create(void *uniform cppE)
{
  Instance *uniform inst = uniform new Instance;

  inst->cppEquivalent = cppE;
  inst->group = NULL;

  return (void *uniform)inst;
}

export void Instance_set(void *uniform _inst,
    void *uniform _group,
    const uniform AffineSpace3f &xfm,
    const uniform AffineSpace3f &rcp_xfm)
{
  Instance *uniform inst = (Instance * uniform) _inst;

  inst->group = (Group * uniform) _group;

  inst->xfm = xfm;
  inst->rcp_xfm = rcp_xfm;
}
