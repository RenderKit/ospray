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

#include "volume/Volume.ih"

unmasked void Volume_embreeBounds(
    const RTCBoundsFunctionArguments *uniform args)
{
  Volume *uniform self = (Volume * uniform) args->geometryUserPtr;
  box3fa *uniform out  = (box3fa * uniform) args->bounds_o;

  *out = make_box3fa(self->boundingBox);
}

unmasked void Volume_intersect_kernel(
    const RTCIntersectFunctionNArguments *uniform args)
{
  // make sure to set the mask
  if (!args->valid[programIndex])
    return;

  Volume *uniform self     = (Volume * uniform) args->geometryUserPtr;
  varying Ray *uniform ray = (varying Ray * uniform) args->rayhit;

  EmbreeIntersectionContext *uniform ctx =
      (EmbreeIntersectionContext * uniform) args->context;

  varying VolumeIntervals *uniform vIntervals =
      (varying VolumeIntervals * uniform) ctx->userPtr;
  
  varying VolumeInterval *varying vInterval =
      (varying VolumeInterval * varying) (vIntervals->intervals + vIntervals->numVolumeIntervals);

  uniform int primID = args->primID;

  box3f *uniform box = &self->boundingBox;

  IntersectionValues isect = intersectBox(ray->org, ray->dir, *box);

  const bool hit = isect.t_in.t_value < isect.t_out.t_value &&
          ((isect.t_in.t_value > ray->t0 && isect.t_in.t_value < ray->t) ||
           (isect.t_out.t_value > ray->t0 && isect.t_out.t_value < ray->t));

  if (hit) {
    ray->instID = args->context->instID[0];
    ray->geomID = self->volumeID;
    ray->primID = 0;

    vInterval->primID = 0;
    vInterval->geomID = self->volumeID;
    vInterval->instID = args->context->instID[0];
    vIntervals->numVolumeIntervals++;
    vInterval->interval.lower = isect.t_in.t_value;
    vInterval->interval.upper = isect.t_out.t_value;

  }
}

void Volume_Constructor(Volume *uniform self, void *uniform cppEquivalent)
{
  self->cppEquivalent = cppEquivalent;

  // default sampling step; should be set to correct value by derived volume.
  self->samplingStep = 1.f;

  // default bounding box; should be set to correct value by derived volume.
  self->boundingBox = make_box3f(make_vec3f(0.f), make_vec3f(1.f));

  self->volumeID = -1;
}

export void Volume_getBoundingBox(uniform box3f *uniform ret,
                                  void *uniform _self)
{
  uniform Volume *uniform self = (uniform Volume * uniform) _self;

  *ret = self->boundingBox;
}

export void Volume_set(void *uniform _self,
                       void *uniform _embreeGeometry)
{
  Volume *uniform self = (Volume * uniform) _self;

  RTCGeometry geom = (RTCGeometry)_embreeGeometry;

  rtcSetGeometryUserData(geom, self);
  rtcSetGeometryUserPrimitiveCount(geom, 1);
  rtcSetGeometryBoundsFunction(
      geom, (uniform RTCBoundsFunction)&Volume_embreeBounds, self);
  rtcSetGeometryIntersectFunction(
      geom, (uniform RTCIntersectFunctionN)&Volume_intersect_kernel);
  rtcCommitGeometry(geom);
}

export void Volume_set_geomID(void *uniform _self, uniform int32 geomID)
{
  Volume *uniform self = (Volume *uniform)_self;

  self->volumeID = geomID;
}
