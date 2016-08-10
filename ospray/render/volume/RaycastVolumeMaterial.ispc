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

#include "render/volume/RaycastVolumeMaterial.ih"

export void *uniform RaycastVolumeMaterial_create(void *uniform cppE)
{
  uniform RaycastVolumeRendererMaterial *uniform mat = uniform new uniform RaycastVolumeRendererMaterial;
  mat->super.cppEquivalent = cppE;
  return mat;
}

export void RaycastVolumeMaterial_set(void *uniform _mat,
                                              const void *uniform map_d,
                                              const uniform float &d,
                                              const void *uniform map_Kd,
                                              const uniform vec3f &Kd,
                                              const void *uniform map_Ks,
                                              const uniform vec3f &Ks,
                                              const void *uniform map_Ns,
                                              const uniform float &Ns,
                                              const void *uniform map_Bump,
                                              void *uniform _volume)
{
  uniform RaycastVolumeRendererMaterial *uniform self = (uniform RaycastVolumeRendererMaterial *uniform)_mat;
  uniform Volume *uniform volume = (uniform Volume *uniform)_volume;

  self->map_d = (uniform Texture2D *uniform)map_d;
  self->d = d;
  self->map_Kd = (uniform Texture2D *uniform)map_Kd;
  self->Kd = Kd;
  self->map_Ks = (uniform Texture2D *uniform)map_Ks;
  self->Ks = Ks;
  self->map_Ns = (uniform Texture2D *uniform)map_Ns;
  self->Ns = Ns;
  self->map_Bump = (uniform Texture2D *uniform)map_Bump;
  self->volume = volume;
}
