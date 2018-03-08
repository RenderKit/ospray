// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

/*! \file ospray/moduleInit \brief Defines the module initialization callback */

#include "geometry/BilinearPatches.h"

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
    extern "C" void ospray_init_module_bilinear_patches()
    {
      std::cout << "#osp: initializing the 'bilinear_patches' module" << std::endl;
      /* nothing to do, actually - this is only an example */
    }

  } // ::ospray::bilinearPatch
} // ::ospray

