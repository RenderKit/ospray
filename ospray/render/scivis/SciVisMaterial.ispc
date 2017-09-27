// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "SciVisMaterial.ih"

export void *uniform SciVisMaterial_create(void *uniform cppE)
{
  uniform SciVisMaterial *uniform mat = uniform new uniform SciVisMaterial;
  mat->super.cppEquivalent = cppE;
  return mat;
}

export void SciVisMaterial_set(void *uniform _mat,
                               const void *uniform map_d,
                               const uniform affine2f &xform_d,
                               const uniform float &d,
                               const void *uniform map_Kd,
                               const uniform affine2f &xform_Kd,
                               const uniform vec3f &Kd,
                               const void *uniform map_Ks,
                               const uniform affine2f &xform_Ks,
                               const uniform vec3f &Ks,
                               const void *uniform map_Ns,
                               const uniform affine2f &xform_Ns,
                               const uniform float &Ns,
                               const void *uniform map_Bump,
                               const uniform affine2f &xform_Bump,
                               const uniform linear2f &rot_Bump,
                               void *uniform _volume)
{
  SciVisMaterial *uniform self = (SciVisMaterial *uniform)_mat;
  Volume *uniform volume = (Volume *uniform)_volume;

  self->map_d = make_TextureParam((Texture2D*)map_d, xform_d);
  self->d = d;
  self->map_Kd = make_TextureParam((Texture2D*)map_Kd, xform_Kd);
  self->Kd = Kd;
  self->map_Ks = make_TextureParam((Texture2D*)map_Ks, xform_Ks);
  self->Ks = Ks;
  self->map_Ns = make_TextureParam((Texture2D*)map_Ns, xform_Ns);
  self->Ns = Ns;
  self->map_Bump = make_TextureParam((Texture2D*)map_Bump, xform_Bump);
  self->rot_Bump = rot_Bump;
  self->volume = volume;
}
