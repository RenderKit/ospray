// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BilinearPatches.h"
// 'export'ed functions from the ispc file:
#include "geometry/BilinearPatches_ispc.h"
// ospray core:
#include <common/Data.h>

// _everything_ in the ospray core universe should _always_ be in the
// 'ospray' namespace.
namespace ospray {

// though not required, it is good practice to put any module into
// its own namespace (isnide of ospray:: ). Unlike for the naming of
// library and init function, the naming for this namespace doesn't
// particularlly matter. E.g., 'bilinearPatch', 'module_blp',
// 'bilinar_patch' etc would all work equally well.
namespace blp {

BilinearPatches::BilinearPatches()
{
  getSh()->super.postIntersect = ispc::BilinearPatches_postIntersect_addr();
}

// commit - this is the function that parses all the parameters
// that the app has proivded for this geometry. In this simple
// example we're looking for a single parameter named 'patches',
// which is supposed to contain a data array of all the patches'
// control points.
void BilinearPatches::commit()
{
  patchesData = getParamDataT<vec3f>("vertices", true);
  if (!patchesData && !patchesData->compact())
    throw std::runtime_error("BilinearPatches needs compact 'vertices' data!");

  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::BilinearPatches_bounds,
      (RTCIntersectFunctionN)&ispc::BilinearPatches_intersect,
      (RTCOccludedFunctionN)&ispc::BilinearPatches_occluded);
  getSh()->patchArray = (ispc::Patch *)patchesData->data();
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo(patchesData->size());
}

size_t BilinearPatches::numPrimitives() const
{
  return patchesData->size() * sizeOf(patchesData->type) / sizeof(ispc::Patch);
}

} // namespace blp
} // namespace ospray
