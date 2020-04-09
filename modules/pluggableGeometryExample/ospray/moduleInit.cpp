// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/*! \file ospray/moduleInit \brief Defines the module initialization callback */

#include "geometry/BilinearPatches.h"
#include "ospray/version.h"

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {

/*! though not required, it is good practice to put any module into
  its own namespace (isnide of ospray:: ). Unlike for the naming of
  library and init function, the naming for this namespace doesn't
  particularlly matter. E.g., 'bilinearPatch', 'module_blp',
  'bilinar_patch' etc would all work equally well. */
namespace blp {

/*! the actual module initialization function. This function gets
    called exactly once, when the module gets first loaded through
    'ospLoadModule'. Notes:

    a) this function does _not_ get called if the application directly
    links to libospray_module_<modulename> (which it
    shouldn't!). Modules should _always_ be loaded through
    ospLoadModule.

    b) it is _not_ valid for the module to do ospray _api_ calls
    inside such an intiailzatoin function. Ie, you can _not_ do a
    ospLoadModule("anotherModule") from within this function (but
    you could, of course, have this module dynamically link to the
    other one, and call its init function)

    c) in order for ospray to properly resolve that symbol, it
    _has_ to have extern C linkage, and it _has_ to correspond to
    name of the module and shared library containing this module
    (see comments regarding library name in CMakeLists.txt)
*/

extern "C" OSPError OSPRAY_DLLEXPORT ospray_module_init_bilinear_patches(
    int16_t versionMajor, int16_t versionMinor, int16_t /*versionPatch*/)
{
  auto status = moduleVersionCheck(versionMajor, versionMinor);

  if (status == OSP_NO_ERROR) {
    /*! maybe one of the most important parts of this example: this
        function 'registers' the BilinearPatches class under the ospray
        geometry type name of 'bilinear_patches'.

        It is _this_ name that one can now (assuming the module has
        been loaded with ospLoadModule(), of course) create geometries
        with; i.e.,

        OSPGeometry geom = ospNewGeometry("bilinear_patches") ;
    */
    Geometry::registerType<BilinearPatches>("bilinear_patches");
  }

  return status;
}

} // namespace blp
} // namespace ospray
