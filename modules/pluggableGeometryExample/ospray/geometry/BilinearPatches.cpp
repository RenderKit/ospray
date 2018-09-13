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

    /*! constructor - will create the 'ispc equivalent' */
    BilinearPatches::BilinearPatches()
    {
      /*! create the 'ispc equivalent': ie, the ispc-side class that
        implements all the ispc-side code for intersection,
        postintersect, etc. See BilinearPatches.ispc */
      this->ispcEquivalent = ispc::BilinearPatches_create(this);

      // note we do _not_ yet do anything else here - the actual input
      // data isn't available to use until 'commit()' gets called
    }

    /*! destructor - supposed to clean up all alloced memory */
    BilinearPatches::~BilinearPatches()
    {
      ispc::BilinearPatches_destroy(ispcEquivalent);
    }

    /*! commit - this is the function that parses all the parameters
      that the app has proivded for this geometry. In this simple
      example we're looking for a single parameter named 'patches',
      which is supposed to contain a data array of all the patches'
      control points */
    void BilinearPatches::commit()
    {
      this->patchesData = getParamData("vertices");

      /* assert that some valid input data is available */
      if (!this->patchesData) {

        std::cout << "#osp.blp: Warning: no input patches provided "
                  << "for bilinear_patches geometry" << std::endl;
        return;
      }
    }

    /*! 'finalize' is what ospray calls when everything is set and
        done, and a actual user geometry has to be built */
    void BilinearPatches::finalize(Model *model)
    {
      // sanity check if a patches data was actually set!
      if (!patchesData)
        return;

      Geometry::finalize(model);

      // look at the data we were provided with ....
      size_t numPatchesInInput = patchesData->numBytes / sizeof(Patch);

      /* get the acual 'raw' pointer to the data (ispc doesn't konw
         what to do with the 'Data' abstraction calss */
      void *patchesDataPointer = patchesData->data;
      ispc::BilinearPatches_finalize(getIE(),model->getIE(),
                                     (float*)patchesDataPointer,
                                     numPatchesInInput);

      /* important: set our bounding box, else renderer can't compute
         global bounding box, and may get confused with epsilon
         compuatations */
      bounds = empty;
      const Patch *patch = (const Patch *)patchesDataPointer;
      for (uint32_t i = 0; i < numPatchesInInput; i++) {
        bounds.extend(patch[i].controlPoint[0][0]);
        bounds.extend(patch[i].controlPoint[0][1]);
        bounds.extend(patch[i].controlPoint[1][0]);
        bounds.extend(patch[i].controlPoint[1][1]);
      }
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

  } // ::ospray::bilinearPatch
} // ::ospray
