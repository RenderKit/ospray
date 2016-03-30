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

#include "Distribution2D.ih"

// input: importance in cdf
// output: cdf of importance in cdf
//         return sum
uniform float Distribution1D_create(const uniform int size, uniform float* uniform cdf)
{
  // accumulate the function
  // using varying float and exclusive_scan_add(float v) is much slower
  uniform float sum = cdf[0];
  for (uniform int i = 1; i < size; i++)
    cdf[i] = sum += cdf[i];

  // compute reciprocal sum
  const uniform float rcpSum = 1.0f/sum;
  // next representable number in float greater than 1.0f
  const uniform float nextAfter1 = 0x1.000002p+0f;

  // normalize
  foreach(i = 0 ... size) {
    const float c = cdf[i];
    // handle cornercases (sum=0 -> rcpSum=inf; correct termination of sampling with s=1.0f)
    const float q = c >= sum ? nextAfter1 : c * rcpSum;
    cdf[i] = q;
  }

  return sum;
}

struct Sample1D {
  int idx; // relative to start
  float frac;
  float pdf;
};

inline Sample1D Distribution1D_sample(
  const uniform int size,
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

  Sample1D ret;
  ret.idx = first - start;
  const float bef = first == start ? 0.0f : cdf[first-1];
  const float dcdf = cdf[first] - bef;
  ret.pdf = dcdf * size;
  ret.frac = (s - bef) * rcp(dcdf); // rescale

  return ret;
}

Sample2D Distribution2D_sample(const uniform Distribution2D* uniform self, const vec2f &s) 
{
  // use u.y to sample a row
  const Sample1D sy = Distribution1D_sample(self->size.y, self->cdf_y, 0, s.y);

  // use u.x to sample inside the row
  const int x0 = sy.idx * self->size.x;
  const Sample1D sx = Distribution1D_sample(self->size.x, self->cdf_x, x0, s.x);

  Sample2D ret;
  ret.uv = make_vec2f((sx.idx + sx.frac)*self->rcpSize.x, (sy.idx + sy.frac)*self->rcpSize.y);
  ret.pdf = sx.pdf * sy.pdf;

  return ret;
}

float Distribution2D_pdf(const uniform Distribution2D* uniform self, const vec2f &uv)
{
  const int idy = uv.y * self->size.y;
  const float befy = idy == 0 ? 0.0f : self->cdf_y[idy-1];
  const float pdf_y = (self->cdf_y[idy] - befy) * self->size.y;

  const int idx = uv.x * self->size.x;
  const int x0 = idy * self->size.x;
  const float befx = idx == 0 ? 0.0f : self->cdf_x[x0 + idx-1];
  const float pdf_x = (self->cdf_x[x0 + idx] - befx) * self->size.x;

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

  self->size = size;
  self->rcpSize = make_vec2f(1.0f/size.x, 1.0f/size.y);

  // create cdf for sampling rows
  uniform float* uniform cdf_y = uniform new uniform float[size.y];

  // compute y distribution and initialize row distributions
  uniform int i = 0;
  for (uniform int y = 0; y < size.y; y++)
  {
    // initialize distribution for current row
    cdf_y[y] = Distribution1D_create(size.x, f + y*size.x);
  }

  // initializes the y distribution
  Distribution1D_create(size.y, cdf_y);

  self->cdf_x = f;
  self->cdf_y = cdf_y;

  return self;
}

