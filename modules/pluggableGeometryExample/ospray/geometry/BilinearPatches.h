// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

/*! \file ospray/geometry/BilinearPatches.h Defines a new ospray
  geometry type (of name 'bilinear_patches'). Input to the geometry is
  a single data array (named 'patches') that consists of for vec3fs
  per patch. */

// ospcomon: vec3f, box3f, etcpp - generic helper stuff
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"
// ospray: everything that's related to the ospray ray tracing core
#include "ospray/geometry/Geometry.h"

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

  BilinearPatches();
  virtual ~BilinearPatches() override = default;

  /*! the commit() message that gets called upon the app calling
      "ospCommit(<thisGeometry>)" */
  virtual void commit() override;

  virtual size_t numPrimitives() const override;

 protected:
  /*! the input data array. the data array contains a list of
      patches, each of which consists of four vec3fs. Note in this
      example we do not particularly care about whether this comes
      as a plain array of floats (with 12 floats per patch), or as
      a array of vec3fs. */
  Ref<const DataT<vec3f>> patchesData;
};

} // namespace blp
} // namespace ospray
