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
#include "Geometry.ih"
#include "common/World.ih"
#include "common/safe_gather.ih"
#include "math/LinearSpace.ih"
#include "math/sampling.ih"

struct Subdivision
{
  Geometry super;
  vec2f *texcoord;
};

static void Subdivision_postIntersect(const Geometry *uniform _self,
                                      const World *uniform model,
                                      varying DifferentialGeometry &dg,
                                      const varying Ray &ray,
                                      uniform int64 flags)
{
  Subdivision *uniform self = (Subdivision * uniform) _self;
  dg.Ng = dg.Ns = ray.Ng;
  // get normals
  cif (flags & DG_NS) {
    vec3f dPdu, dPdv;
    rtcInterpolateV1(self->super.geom,
                     ray.primID,
                     ray.u,
                     ray.v,
                     RTC_BUFFER_TYPE_VERTEX,
                     0,
                     NULL,
                     &dPdu.x,
                     &dPdv.x,
                     3);
    dg.Ns = cross(dPdu, dPdv);
  }
  // get colors
  cif (flags & DG_COLOR) {
    rtcInterpolateV1(self->super.geom,
                     ray.primID,
                     ray.u,
                     ray.v,
                     RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                     0,
                     &dg.color.x,
                     NULL,
                     NULL,
                     4);
  }
  // get texcoords
  cif (flags & DG_TEXCOORD && self->texcoord) {
    rtcInterpolateV1(self->super.geom,
                     ray.primID,
                     ray.u,
                     ray.v,
                     RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                     1,
                     &dg.st.x,
                     NULL,
                     NULL,
                     2);
  }
}

void Subdivision_Constructor(uniform Subdivision *uniform mesh,
                             void *uniform cppEquivalent,
                             RTCGeometry geom,
                             uniform int32 geomID,
                             uniform vec2f *uniform texcoord)
{
  Geometry_Constructor(
      &mesh->super, cppEquivalent, Subdivision_postIntersect, geom, geomID);

  mesh->texcoord = texcoord;
}

export void *uniform Subdivision_create(void *uniform cppEquivalent)
{
  Subdivision *uniform mesh = uniform new Subdivision;
  Subdivision_Constructor(mesh, cppEquivalent, NULL, 0, NULL);
  return mesh;
}

export void *uniform Subdivision_set(void *uniform _mesh,
                                     RTCGeometry geom,
                                     uniform int32 geomID,
                                     uniform vec2f *uniform texcoord)
{
  uniform Subdivision *uniform mesh = (uniform Subdivision * uniform) _mesh;
  Subdivision_Constructor(
      mesh, mesh->super.cppEquivalent, geom, geomID, texcoord);
}
