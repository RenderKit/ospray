// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BilinearPatches.h"
// 'export'ed functions from the ispc file:
#include "geometry/BilinearPatches_ispc.h"
// ospray core:
#include <ospray/common/Data.h>

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {

/*! though not required, it is good practice to put any module into
  its own namespace (isnide of ospray:: ). Unlike for the naming of
  library and init function, the naming for this namespace doesn't
  particularlly matter. E.g., 'bilinearPatch', 'module_blp',
  'bilinar_patch' etc would all work equally well. */
namespace blp {

BilinearPatches::BilinearPatches()
{
  /*! create the 'ispc equivalent': ie, the ispc-side class that
    implements all the ispc-side code for intersection,
    postintersect, etc. See BilinearPatches.ispc */
  ispcEquivalent = ispc::BilinearPatches_create(this);
  embreeGeometry = rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);
}

/*! commit - this is the function that parses all the parameters
  that the app has proivded for this geometry. In this simple
  example we're looking for a single parameter named 'patches',
  which is supposed to contain a data array of all the patches'
  control points */
void BilinearPatches::commit()
{
  this->patchesData = getParamDataT<vec3f>("vertices", true);

  if (!patchesData && !patchesData->compact())
    throw std::runtime_error("BilinearPatches needs compact 'vertices' data!");

  // look at the data we were provided with ....
  size_t numPatchesInInput = numPrimitives();

  ispc::BilinearPatches_finalize(
      getIE(), embreeGeometry, (float *)patchesData->data(), numPatchesInInput);

  postCreationInfo(patchesData->size());
}

size_t BilinearPatches::numPrimitives() const
{
  return patchesData->size() * sizeOf(patchesData->type) / sizeof(Patch);
}

} // namespace blp
} // namespace ospray
