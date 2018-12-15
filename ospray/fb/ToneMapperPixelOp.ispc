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

#include "math/vec.ih"
#include "math/LinearSpace.ih"
#include "Tile.ih"

// Based on the generic filmic tone mapping operator from
// [Lottes, 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"]
struct ToneMapperPixelOp
{
  uniform float exposure;   // linear exposure adjustment
  uniform float a, b, c, d; // coefficients
  uniform bool acesColor;   // ACES color transform flag
};

// ACES input transform matrix = RRT_SAT_MAT * XYZ_2_AP1_MAT * D65_2_D60_CAT * REC709_2_XYZ_PRI_MAT
static const uniform LinearSpace3f acesInputMat = {
  {0.5972782409, 0.0760130499, 0.0284085382},
  {0.3545713181, 0.9083220973, 0.1338243154},
  {0.0482176639, 0.0156579968, 0.8375684636}
};

// ACES output transform matrix = XYZ_2_REC709_PRI_MAT * D60_2_D65_CAT * AP1_2_XYZ_MAT * ODT_SAT_MAT
static const uniform LinearSpace3f acesOutputMat = {
  { 1.6047539945, -0.1020831870, -0.0032670420},
  {-0.5310794927,  1.1081322801, -0.0727552477},
  {-0.0736720338, -0.0060518756,  1.0760219533}
};

inline vec3f toneMap(const ToneMapperPixelOp* uniform self, const vec3f& col)
{
  vec3f x = col * self->exposure;
  if (self->acesColor)
    x = acesInputMat * x;
  x = pow(x, self->a) / (pow(x, self->a * self->d) * self->b + self->c);
  if (self->acesColor)
    x = acesOutputMat * x;
  x = clamp(x, make_vec3f(0.f), make_vec3f(1.f));
  return x;
}

export void* uniform ToneMapperPixelOp_create()
{
  ToneMapperPixelOp* uniform self = uniform new uniform ToneMapperPixelOp;
  self->exposure = 1.f;
  self->a = 1.f;
  self->b = 1.f;
  self->c = 0.f;
  self->d = 1.f;
  self->acesColor = false;
  return self;
}

export void ToneMapperPixelOp_set(void* uniform _self,
                                  uniform float exposure,
                                  uniform float a, uniform float b, uniform float c, uniform float d,
                                  uniform bool acesColor)
{
  ToneMapperPixelOp* uniform self = (ToneMapperPixelOp* uniform)_self;
  self->exposure = exposure;
  self->a = a;
  self->b = b;
  self->c = c;
  self->d = d;
  self->acesColor = acesColor;
}

export void ToneMapperPixelOp_apply(const void* uniform _self,
                                    uniform Tile& tile)
{
  const ToneMapperPixelOp* uniform self = (const ToneMapperPixelOp* uniform)_self;
  VaryingTile* uniform varyTile = (VaryingTile* uniform)&tile;
  
  for (uniform uint32 iy = 0; iy < TILE_SIZE; iy++) {
    uniform uint32 iiy = tile.region.lower.y + iy;
    if (iiy >= tile.region.upper.y) continue;

    uniform uint32 chunkID = iy*(TILE_SIZE/programCount);

    for (uint32 iix = tile.region.lower.x + programIndex;
         iix < tile.region.upper.x; iix += programCount, chunkID++) {
      unmasked {
        varying vec3f col = make_vec3f(varyTile->r[chunkID],
                                       varyTile->g[chunkID],
                                       varyTile->b[chunkID]);
                                       
        col = toneMap(self, col);
        
        varyTile->r[chunkID] = col.x;
        varyTile->g[chunkID] = col.y;
        varyTile->b[chunkID] = col.z;
      }
    }
  }
}
