// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include "ospray/render/volume/RaycastVolumeRendererMaterial.ih"

export void *uniform RaycastVolumeRendererMaterial_create(void *uniform cppE)
{
  uniform RaycastVolumeRendererMaterial *uniform mat = uniform new uniform RaycastVolumeRendererMaterial;
  mat->inherited.cppEquivalent = cppE;
  return mat;
}

export void RaycastVolumeRendererMaterial_set(void *uniform _mat,
                                              const uniform vec3f &Kd,
                                              void *uniform _volume)
{
  uniform RaycastVolumeRendererMaterial *uniform mat = (uniform RaycastVolumeRendererMaterial *uniform)_mat;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  mat->Kd = Kd;
  mat->volume = volume;
}
