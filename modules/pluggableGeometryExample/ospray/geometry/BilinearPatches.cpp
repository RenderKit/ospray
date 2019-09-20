// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
      this->patchesData = getParamData("vertices");

      // sanity check if a patches data was actually set!
      if (!patchesData)
        throw std::runtime_error("no data provided to BilinearPatches!");
    }

    size_t BilinearPatches::numPrimitives() const
    {
      return patchesData->size() * sizeOf(patchesData->type) / sizeof(Patch);
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

      // look at the data we were provided with ....
      size_t numPatchesInInput = numPrimitives();

      /* get the acutal 'raw' pointer to the data (ispc doesn't know
         what to do with the 'Data' abstraction class */
      void *patchesDataPointer = patchesData->data();

      ispc::BilinearPatches_finalize(retval.ispcEquivalent,
                                     retval.embreeGeometry,
                                     (float *)patchesDataPointer,
                                     numPatchesInInput);

      return retval;
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

  }  // namespace blp
}  // namespace ospray
