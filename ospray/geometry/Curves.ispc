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

// ospray
#include "math/vec.ih"
#include "math/box.ih"
#include "common/Ray.ih"
#include "common/Model.ih"
#include "geometry/Geometry.ih"
#include "math/sampling.ih"

struct Curves
{
  uniform Geometry super; //!< inherited geometry fields
};

static void Curves_postIntersect(uniform Geometry *uniform self,
                                 uniform Model *uniform model,
                                 varying DifferentialGeometry &dg,
                                 const varying Ray &ray,
                                 uniform int64 flags)
{
  dg.Ng = dg.Ns = ray.Ng;
}

export void *uniform Curves_create(void *uniform cppEquivalent)
{
  Curves *uniform geom = uniform new Curves;
  Geometry_Constructor(&geom->super,
                       cppEquivalent,
                       Curves_postIntersect,
                       NULL,
                       NULL,
                       0,
                       NULL);
  return geom;
}

export void *uniform
Curves_set(void           *uniform _self,
           void           *uniform _model,
           bool           uniform flat,
           const uniform vec4f *uniform vertexCurve,
           int32           uniform numVertices,
           const uniform uint32 *uniform indexCurve,
           int32           uniform numSegments)
{
  Curves *uniform self = (Curves *uniform)_self;
  Model *uniform model = (Model *uniform)_model;

  uniform RTCGeometry geom
    = rtcNewGeometry(ispc_embreeDevice(),
                     flat ? RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE
                          : RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE);
  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4,
                             vertexCurve, 0, sizeof(uniform vec4f),numVertices);
  rtcSetSharedGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT,
                             indexCurve, 0, sizeof(uniform int), numSegments);
  uniform uint32 geomID = rtcAttachGeometry(model->embreeSceneHandle, geom);
  rtcCommitGeometry(geom);
  rtcReleaseGeometry(geom);

  self->super.geom = geom;

  self->super.model = model;
  self->super.geomID = geomID;
  self->super.numPrimitives = numSegments;
  self->super.postIntersect = Curves_postIntersect;
  // TODO implement area sampling of curves for geometry lights
  self->super.getAreas = NULL;
  self->super.sampleArea = NULL;
}
