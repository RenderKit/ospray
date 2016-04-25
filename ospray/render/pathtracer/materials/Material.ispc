// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "Material.ih"

vec3f PathTraceMaterial_getTransparency(const uniform PathTraceMaterial* uniform self,
                                        const DifferentialGeometry& dg,
                                        const Ray& ray,
                                        const Medium& currentMedium)
{
  return make_vec3f(0.0f);
}

void PathTraceMaterial_selectNextMedium(const uniform PathTraceMaterial* uniform self,
                                        Medium& currentMedium)
{ /* do nothing by default */ }

void PathTraceMaterial_Constructor(uniform PathTraceMaterial* uniform self,
                                   uniform PathTraceMaterial_GetBSDFFunc getBSDF,
                                   uniform PathTraceMaterial_GetTransparencyFunc getTransparency,
                                   uniform PathTraceMaterial_SelectNextMediumFunc selectNextMedium)
{
  self->getBSDF = getBSDF;
  self->getTransparency = getTransparency ? getTransparency : PathTraceMaterial_getTransparency;
  self->selectNextMedium = selectNextMedium ? selectNextMedium : PathTraceMaterial_selectNextMedium;
}
