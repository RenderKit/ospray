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

#include "Distribution2D.ih"

// input: importance in cdf
// output: cdf of importance in cdf
void Distribution1D_create(const uniform int size, uniform float* uniform cdf)
{
  // TODO: maybe use SIMD (varying float) and float exclusive_scan_add(float v)
  // accumulate the function
  uniform float sum = cdf[0];
  for (uniform int i = 1; i < size; i++)
    cdf[i] = sum += cdf[i];
  
  // compute reciprocal sum
  uniform float rcpSum = sum == 0.0f ? 0.0f : 1.0f/sum;

  // normalize
  foreach(i = 0 ... size-1)
    cdf[i] *= rcpSum;

  cdf[size-1] = 1.0f;
}

// what if sum = 0?
//         s = 1.0f

struct Sample1D {
  int idx;
  float frac;
  float pdf;
};

inline Sample1D Distribution1D_sample(const uniform int size,
                                      const uniform float* uniform cdf,
// we may sample different rows within one gang: use a varying offset 'start' instead of a varying pointer 'cdf'
                                      const int start,
                                      const float s)
{
  // find minimum index where cdf[i-1] <= s < cdf[i]
  int first = start;
  int len = size;

  while (len > 0) {
    const int half = len >> 1;
    const int mid = first + half;
    if (s < cdf[mid]) {
      len = half;
    } else {
      first = mid + 1;
      len -= half + 1;
    }
  }
  
  // sample
//  const int index = clamp(first-1, 0, size-2);
  const float bef = first == 0 ? 0.0f : cdf[first-1];
  float pdf = cdf[first] - bef; // actually / size
  const float fraction = (s - bef) * rcp(pdf);

  Sample1D ret;
  ret.idx = first;
  ret.frac = fraction;
  ret.pdf = pdf;

  return ret;
}

vec2f Distribution2D_sample(const uniform Distribution2D* uniform self, const vec2f &s) 
{
  // use u.y to sample a row
  const Sample1D sy = Distribution1D_sample(self->size.y, self->cdf_y, 0, s.y);
  
  // use u.x to sample inside the row
  const int x0 = sy.idx * self->size.x;
  const Sample1D sx = Distribution1D_sample(self->size.x, self->cdf_x, x0, s.x);

  return make_vec2f((sx.idx + sx.frac)/self->size.x, (sy.idx + sy.frac)/self->size.y);
}

float Distribution2D_pdf(const uniform Distribution2D* uniform self, const vec2f &uv)
{
  const int idy = uv.y * self->size.y;
  const float pdf_y = self->cdf_y[idy+1] - self->cdf_y[idy];

  const int idx = idy * self->size.x + uv.x * self->size.x;
  const float pdf_x = self->cdf_x[idx+1] - self->cdf_x[idx];

  return pdf_x * pdf_y;
}

void Distribution2D_destroy(uniform Distribution2D* uniform self)
{ 
  delete[] self->cdf_x;
  delete[] self->cdf_y;
  delete self;
}

uniform Distribution2D* uniform Distribution2D_create(const uniform vec2i size, uniform float* uniform f)
{
  uniform Distribution2D* uniform self = uniform new uniform Distribution2D;

  // create cdf for sampling rows
  uniform float* uniform cdf_y = uniform new uniform float[size.y];
  
  // compute y distribution and initialize row distributions
  uniform int i = 0;
  for (uniform int y = 0; y < size.y; y++)
  {
    // initialize distribution for current row
    Distribution1D_create(size.x, f + y*size.x);
    cdf_y[y] = f[size.x + y*size.x - 1];
  }
  
  // initializes the y distribution
  Distribution1D_create(size.y, cdf_y);

  self->size  = size;
  self->cdf_x = f;
  self->cdf_y = cdf_y;

  return self;
}

