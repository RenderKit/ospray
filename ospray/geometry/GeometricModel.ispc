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
#include "GeometricModel.ih"
#include "common/Ray.ih"
#include "math/box.ih"
#include "math/vec.ih"

export void *uniform GeometricModel_create(void *uniform cppE)
{
  GeometricModel *uniform self = uniform new GeometricModel;

  self->geom = NULL;
  Data1D_Constructor(&self->color);
  Data1D_Constructor(&self->index);
  self->numMaterials = 0;
  self->material = NULL;
  self->areaPDF = 0.f;

  return self;
}

export void GeometricModel_set(void *uniform _self,
    const Data1D *uniform color,
    const Data1D *uniform index,
    uniform int numMaterials,
    void *uniform _matList)
{
  GeometricModel *uniform self = (GeometricModel * uniform) _self;

  self->color = *color;
  self->index = *index;
  self->numMaterials = numMaterials;
  self->material = (Material * uniform * uniform) _matList;
}

export void GeometricModel_setGeomIE(void *uniform _self, void *uniform _geomIE)
{
  GeometricModel *uniform self = (GeometricModel * uniform) _self;

  self->geom = (Geometry * uniform) _geomIE;
}
