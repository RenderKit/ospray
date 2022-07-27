// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "GeometricModel.ih"
#include "math/Distribution1D.ih"
#include "render/Material.ih"
// c++ shared
#include "common/InstanceShared.h"

export uniform int GeometricModel_gatherEmissivePrimIDs(void *uniform _model,
    void *uniform _renderer,
    void *uniform _instance,
    int *uniform primIDs,
    float *uniform distribution,
    uniform float &pdf)
{
  GeometricModel *uniform model = (GeometricModel * uniform) _model;
  const Renderer *uniform renderer = (Renderer * uniform) _renderer;
  const Geometry *uniform geo = model->geom;
  const Instance *uniform instance = (Instance * uniform) _instance;

  // create the list of emissive primitive IDs
  uniform int32 numEmissivePrims = 0;
  foreach (primID = 0 ... geo->numPrimitives) {
    Material *mat =
        (Material *)GeometricModel_getMaterial(model, renderer, primID);
    const vec3f emission = mat->emission;
    if (reduce_max(emission) > 0.f) {
      int offset = exclusive_scan_add(1);

      primIDs[numEmissivePrims + offset] = primID;
      numEmissivePrims += reduce_add(1);
    }
  }

  // create the sampling distribution
  // TODO: use emissive power instead of just area
  // TODO: motion blur can introduce scale, which means areas cannot accurately
  // be precomputed
  geo->getAreas(geo, primIDs, numEmissivePrims, instance->xfm, distribution);
  pdf = model->areaPDF =
      1.f / Distribution1D_create(numEmissivePrims, distribution);

  return numEmissivePrims;
}
