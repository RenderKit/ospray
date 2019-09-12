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
#include "math/vec.ih"
#include "common/Intersect.ih"

struct Boxes
{
  Geometry super;
  box3f *boxes;
};

unmasked void Boxes_bounds(const RTCBoundsFunctionArguments *uniform args)
{
  Boxes *uniform self = (Boxes * uniform) args->geometryUserPtr;
  uniform int primID  = args->primID;

  box3fa *uniform out = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(self->boxes[primID]);
}

void Boxes_intersect_kernel(const RTCIntersectFunctionNArguments *uniform args,
                            const uniform bool isOcclusionTest)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Boxes *uniform self      = (Boxes * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  uniform int primID = args->primID;

  box3f *uniform box = &self->boxes[primID];

  IntersectionValues isect = intersectBox(ray->org, ray->dir, *box);

  bool hit = false;
  if(isect.t_in.t_value < isect.t_out.t_value) {
    if (and(and(isect.t_in.t_value > ray->t0, isect.t_in.t_value < ray->t),
            isect.t_in.hit)) {
      hit = true;
      ray->Ng = isect.t_in.N;
      ray->t  = isect.t_in.t_value;
    } else if (and(and(isect.t_out.t_value > ray->t0, isect.t_out.t_value < ray->t),
                   isect.t_out.hit)) {
      hit = true;
      ray->Ng = isect.t_out.N;
      ray->t  = isect.t_out.t_value;
    }
  }

  if (hit) {
    cif(isOcclusionTest)
    {
      ray->t = neg_inf;
    }
    else
    {
      ray->instID = args->context->instID[0];
      ray->geomID = self->super.geomID;
      ray->primID = primID;
    }
  }
}

unmasked void Boxes_intersect(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel(args, false);
}

unmasked void Boxes_occluded(
    const struct RTCIntersectFunctionNArguments *uniform args)
{
  Boxes_intersect_kernel(args, true);
}

static void Boxes_postIntersect(const Geometry *uniform geometry,
                                varying DifferentialGeometry &dg,
                                const varying Ray &ray,
                                uniform int64 flags)
{
  uniform Boxes *uniform self = (uniform Boxes * uniform) geometry;

  dg.Ng = dg.Ns = ray.Ng;

  dg.epsilon = 1e-3f;  // TODO: make this smarter...
}

export void *uniform Boxes_create(void *uniform cppEquivalent)
{
  uniform Boxes *uniform boxes = uniform new uniform Boxes;

  Geometry_Constructor(&boxes->super, cppEquivalent, Boxes_postIntersect, NULL);

  return boxes;
}

export void *uniform Boxes_set(void *uniform _self,
                               void *uniform _embreeGeometry,
                               int32 uniform numBoxes,
                               void *uniform boxes)
{
  Boxes *uniform self = (Boxes * uniform) _self;

  self->super.numPrimitives = numBoxes;
  self->boxes               = (box3f * uniform) boxes;

  RTCGeometry geom = (RTCGeometry)_embreeGeometry;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, numBoxes);
  rtcSetGeometryBoundsFunction(
      geom, (uniform RTCBoundsFunction)&Boxes_bounds, self);
  rtcSetGeometryIntersectFunction(
      geom, (uniform RTCIntersectFunctionN)&Boxes_intersect);
  rtcSetGeometryOccludedFunction(geom,
                                 (uniform RTCOccludedFunctionN)&Boxes_occluded);
  rtcCommitGeometry(geom);
}
