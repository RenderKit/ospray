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
#include "GeometryInstance.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

static void GeometryInstance_postIntersect(const GeometryInstance *uniform self,
                                           const World *uniform parentWorld,
                                           varying DifferentialGeometry &dg,
                                           const varying Ray &ray,
                                           uniform int64 flags)
{
  Geometry *uniform instancedGeometry = self->geom;

  dg.geometry = instancedGeometry;

  instancedGeometry->postIntersect(
      instancedGeometry, parentWorld, dg, ray, flags);

  cif(flags & DG_COLOR && self->primitiveColors)
  {
    dg.color = self->primitiveColors[ray.primID];
  }

  dg.materialID = -1;
  cif(self->prim_materialID)
  {
    dg.materialID = self->prim_materialID[ray.primID];
  }
  else if (self->numMaterials > 1)
  {
    dg.materialID = ray.primID;
  }

  cif(self->materialList)
  {
    dg.material = self->materialList[dg.materialID < 0 ? 0 : dg.materialID];
  }

  dg.Ns = xfmVector(transposed(self->rcp_xfm.l), dg.Ns);
  dg.Ng = xfmVector(transposed(self->rcp_xfm.l), dg.Ng);
  // scale dg.epsilon by max (epsilon is scalar and thus assumed to be
  // isotropic anyway and hence cannot better handle non-uniform scaling)
  dg.epsilon *= max(abs(self->xfm.l.vx.x),
                    max(abs(self->xfm.l.vy.y), abs(self->xfm.l.vz.z)));

  cif(flags & DG_TANGENTS)
  {
    dg.dPds = xfmVector(self->xfm, dg.dPds);
    dg.dPdt = xfmVector(self->xfm, dg.dPdt);
  }
}

export void *uniform GeometryInstance_create(void *uniform cppE,
                                             void *uniform geom)
{
  GeometryInstance *uniform self = uniform new GeometryInstance;

  self->geom    = (Geometry * uniform) geom;
  self->areaPDF = 0.f;

  return self;
}

export void GeometryInstance_set(void *uniform _self,
                                 const uniform AffineSpace3f &xfm,
                                 const uniform AffineSpace3f &rcp_xfm,
                                 void *uniform primitiveColors,
                                 void *uniform prim_materialID)
{
  GeometryInstance *uniform self = (GeometryInstance * uniform) _self;

  self->xfm     = xfm;
  self->rcp_xfm = rcp_xfm;

  self->primitiveColors = (vec4f * uniform) primitiveColors;
  self->prim_materialID = (uint32 * uniform) prim_materialID;
}

export void GeometryInstance_setMaterialList(void *uniform _inst,
                                             uniform int numMaterials,
                                             void *uniform _matList)
{
  GeometryInstance *uniform inst = (GeometryInstance * uniform) _inst;
  inst->numMaterials             = numMaterials;
  inst->materialList             = (Material * uniform * uniform) _matList;
}
