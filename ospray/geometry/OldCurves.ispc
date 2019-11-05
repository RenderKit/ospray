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

// ospray
#include "common/Ray.ih"
#include "common/World.ih"
#include "geometry/Geometry.ih"
#include "math/box.ih"
#include "math/sampling.ih"
#include "math/vec.ih"

struct OldCurves
{
  Geometry super;  //!< inherited geometry fields
};

static void OldCurves_postIntersect(const Geometry *uniform self,
                                 varying DifferentialGeometry &dg,
                                 const varying Ray &ray,
                                 uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform OldCurves_create(void *uniform cppEquivalent)
{
  OldCurves *uniform geom = uniform new OldCurves;
  Geometry_Constructor(&geom->super, cppEquivalent, OldCurves_postIntersect);
  return geom;
}

export void *uniform OldCurves_set(void *uniform _self,
                                RTCGeometryType uniform curveType,
                                int32 uniform numSegments)
{
  OldCurves *uniform self = (OldCurves * uniform) _self;

  self->super.numPrimitives = numSegments;
  self->super.postIntersect = OldCurves_postIntersect;

  // TODO implement area sampling of OldCurves for geometry lights
  self->super.getAreas   = NULL;
  self->super.sampleArea = NULL;
}
