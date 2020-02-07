// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Tile.ih"
#include "math/LinearSpace.ih"
#include "math/vec.ih"

// Based on the generic filmic tone mapping operator from
// [Lottes, 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"]
struct ToneMapper
{
  float exposure; // linear exposure adjustment
  float a, b, c, d; // coefficients
  bool acesColor; // ACES color transform flag
};

// ACES input transform matrix = RRT_SAT_MAT * XYZ_2_AP1_MAT * D65_2_D60_CAT *
// REC709_2_XYZ_PRI_MAT
static const uniform LinearSpace3f acesInputMat = {
    {0.5972782409, 0.0760130499, 0.0284085382},
    {0.3545713181, 0.9083220973, 0.1338243154},
    {0.0482176639, 0.0156579968, 0.8375684636}};

// ACES output transform matrix = XYZ_2_REC709_PRI_MAT * D60_2_D65_CAT *
// AP1_2_XYZ_MAT * ODT_SAT_MAT
static const uniform LinearSpace3f acesOutputMat = {
    {1.6047539945, -0.1020831870, -0.0032670420},
    {-0.5310794927, 1.1081322801, -0.0727552477},
    {-0.0736720338, -0.0060518756, 1.0760219533}};

inline vec3f toneMap(const ToneMapper *uniform self, const vec3f &col)
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

export void *uniform ToneMapper_create()
{
  ToneMapper *uniform self = uniform new uniform ToneMapper;
  self->exposure = 1.f;
  self->a = 1.f;
  self->b = 1.f;
  self->c = 0.f;
  self->d = 1.f;
  self->acesColor = false;
  return self;
}

export void ToneMapper_set(void *uniform _self,
    uniform float exposure,
    uniform float a,
    uniform float b,
    uniform float c,
    uniform float d,
    uniform bool acesColor)
{
  ToneMapper *uniform self = (ToneMapper * uniform) _self;
  self->exposure = exposure;
  self->a = a;
  self->b = b;
  self->c = c;
  self->d = d;
  self->acesColor = acesColor;
}

export void ToneMapper_apply(const void *uniform _self, uniform Tile &tile)
{
  const ToneMapper *uniform self = (const ToneMapper *uniform)_self;
  VaryingTile *uniform varyTile = (VaryingTile * uniform) & tile;

  for (uniform uint32 iy = 0; iy < TILE_SIZE; iy++) {
    uniform uint32 iiy = tile.region.lower.y + iy;
    if (iiy >= tile.region.upper.y)
      continue;

    uniform uint32 chunkID = iy * (TILE_SIZE / programCount);

    for (uint32 iix = tile.region.lower.x + programIndex;
         iix < tile.region.upper.x;
         iix += programCount, chunkID++) {
      unmasked
      {
        varying vec3f col = make_vec3f(
            varyTile->r[chunkID], varyTile->g[chunkID], varyTile->b[chunkID]);

        col = toneMap(self, col);

        varyTile->r[chunkID] = col.x;
        varyTile->g[chunkID] = col.y;
        varyTile->b[chunkID] = col.z;
      }
    }
  }
}
