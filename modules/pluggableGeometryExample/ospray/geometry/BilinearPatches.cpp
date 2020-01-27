// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BilinearPatches.h"
// 'export'ed functions from the ispc file:
#include "BilinearPatches_ispc.h"
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

  postCreationInfo(patchesData->size());
}

LiveGeometry BilinearPatches::createEmbreeGeometry()
{
  LiveGeometry retval;

  /*! create the 'ispc equivalent': ie, the ispc-side class that
    implements all the ispc-side code for intersection,
    postintersect, etc. See BilinearPatches.ispc */
  retval.ispcEquivalent = ispc::BilinearPatches_create(this);
  retval.embreeGeometry =
      rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_USER);

  ispc::BilinearPatches_finalize(getIE(),
      retval.embreeGeometry,
      (float *)patchesData->data(),
      numPrimitives());

  return retval;
}

size_t BilinearPatches::numPrimitives() const
{
  return patchesData->size() * sizeOf(patchesData->type) / sizeof(Patch);
}

/*! maybe one of the most important parts of this example: this
    macro 'registers' the BilinearPatches class under the ospray
    geometry type name of 'bilinear_patches'.

    It is _this_ name that one can now (assuming the module has
    been loaded with ospLoadModule(), of course) create geometries
    with; i.e.,

    OSPGeometry geom = ospNewGeometry("bilinear_patches") ;
*/
OSP_REGISTER_GEOMETRY(BilinearPatches, bilinear_patches);

} // namespace blp
} // namespace ospray
