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

#pragma once

/*! \file ospray/geometry/BilinearPatches.h Defines a new ospray
  geometry type (of name 'bilinear_patches'). Input to the geometry is
  a single data array (named 'patches') that consists of for vec3fs
  per patch. */

// ospcomon: vec3f, box3f, etcpp - generic helper stuff
#include "ospcommon/vec.h"
#include "ospcommon/box.h"
// ospray: everything that's related to the ospray ray tracing core
#include "ospray/geometry/Geometry.h"
#include "ospray/common/Model.h"

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {

  /*! though not required, it is good practice to put any module into
    its own namespace (isnide of ospray:: ). Unlike for the naming of
    library and init function, the naming for this namespace doesn't
    particularlly matter. E.g., 'bilinearPatch', 'module_blp',
    'bilinar_patch' etc would all work equally well. */
  namespace blp {
    // import ospcommon component - vec3f etc
    using namespace ospcommon;

    /*! a geometry type that implements (a set of) bi-linear
      patches. This implements a new ospray geometry, and as such has
      to

      a) derive from ospray::Geometry
      b) implement a 'commit()' message that parses the
         parameters/data arrays that the app has specified as inputs
      c) create an actual ospray geometry instance with the
         proper intersect() and postIntersect() functions.

      Note that how this class is called does not particularly matter;
      all that matters is under which name it is registered in the cpp
      file (see comments on OSPRAY_REGISTER_GEOMETRY)
    */
    struct BilinearPatches : public ospray::Geometry
    {
      /*! data layout of a single patch. note we do not actually use
          this class anywhere on the c++ side of this example, it is
          only for illustrative purposes. The input data should come
          as a data array of N such patches (we compute N
          automatically based on the size of this array) */
      struct Patch
      {
        vec3f controlPoint[2][2];
      };

      /*! constructor - will create the 'ispc equivalent' */
      BilinearPatches();

      /*! destructor - supposed to clean up all alloced memory */
      virtual ~BilinearPatches() override;

      /*! the commit() message that gets called upon the app calling
          "ospCommit(<thisGeometry>)" */
      virtual void commit() override;

      /*! 'finalize' is what ospray calls when everything is set and
        done, and a actual user geometry has to be built */
      virtual void finalize(Model *model) override;

      /*! the input data array. the data array contains a list of
          patches, each of which consists of four vec3fs. Note in this
          example we do not particularly care about whether this comes
          as a plain array of floats (with 12 floats per patch), or as
          a array of vec3fs. */
      Ref<Data> patchesData;
    };

  } // ::ospray::bilinearPatch
} // ::ospray

