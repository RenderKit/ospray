// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Instance.ih"

export void *uniform Instance_create()
{
  Instance *uniform inst = uniform new Instance;

  inst->group = NULL;

  return (void *uniform)inst;
}

export void Instance_set(void *uniform _inst,
    void *uniform _group,
    const uniform AffineSpace3f &xfm,
    const uniform bool motionBlur)
{
  Instance *uniform inst = (Instance * uniform) _inst;

  inst->group = (Group * uniform) _group;

  inst->xfm = xfm;
  inst->rcp_xfm = rcp(xfm);
  inst->motionBlur = motionBlur;
}

export void Instance_set_embreeGeom(void *uniform _inst, RTCGeometry geom)
{
  Instance *uniform inst = (Instance * uniform) _inst;
  inst->geom = geom;
  if (inst->motionBlur) {
    inst->xfm = Instance_getTransform(inst, 0.5f); // for SciVis
    inst->rcp_xfm = rcp(inst->xfm);
  }
}
