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

#include "Texture2D.ih"

#include "../volume/Volume.ih"

vec4f TextureVolume_get(const uniform Texture2D *uniform self,
                        const varying DifferentialGeometry &dg)
{
  uniform Volume *uniform volume = (uniform Volume *uniform)self->data;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, dg.P)) {
    const float sample = Volume_getSample(volume, dg.P);
    return make_vec4f(Volume_getColor(volume, sample),
                      Volume_getOpacity(volume, sample));
  } else {
    return make_vec4f(0.f);
  }
}

vec3f TextureVolume_getN(const uniform Texture2D *uniform self,
                         const varying DifferentialGeometry &dg)
{
  uniform Volume *uniform volume = (uniform Volume *uniform)self->data;
  const uniform box3f bounds = volume->boundingBox;

  if (box_contains(bounds, dg.P)) {
    const float sample = Volume_getSample(volume, dg.P);
    return Volume_getColor(volume, sample);
  } else {
    return make_vec3f(0.f);
  }
}

///////////////////////////////////////////////////////////////////////////////

export void *uniform TextureVolume_create(void *uniform volume)
{
  uniform Texture2D *uniform self = uniform new uniform Texture2D;
  self->data = volume;
  self->get = TextureVolume_get;
  self->getNormal = TextureVolume_getN;

  return self;
}
