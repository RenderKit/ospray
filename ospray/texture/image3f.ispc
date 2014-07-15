/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "image.isph"

struct Image3f
{
  Image base;
  vec3f* data;
  bool owned;
};

inline uniform vec4f Image3f__get_nearest_uniform(const uniform Image *uniform _this, const uniform int x, const uniform int y) 
{
  const uniform Image3f *uniform this = (const uniform Image3f *uniform)_this;
  return this->data[x+this->base.size.x*y];
}

inline varying vec4f Image3f__get_nearest_varying(const uniform Image *uniform _this, const varying int x, const varying int y) 
{
  const uniform Image3f *uniform this = (const uniform Image3f *uniform)_this;
  const int idx = x+this->base.size.x*y;
  return this->data[idx];
}

varying vec4f Image3f__get_bilinear_varying(const uniform Image *uniform _this, const varying float x, const varying float y) 
{ 
  const uniform Image3f *uniform this = (const uniform Image3f *uniform)_this;

  /* repeat texture */
  const float xr = (x-floor(x))*(float)this->base.size.x;
  const float yr = (y-floor(y))*(float)this->base.size.y;

  /* calculate interpolation weights */
  const float sx = xr-floor(xr);
  const float sy = yr-floor(yr);

  /* read texels */
  const int x0 = clamp((int)xr,0,this->base.size.x-1);
  const int y0 = clamp((int)yr,0,this->base.size.y-1);
  const int x1 = x0+1 == this->base.size.x ? 0 : x0+1;
  const int y1 = y0+1 == this->base.size.y ? 0 : y0+1;

  const vec3f c00 = Image3f__get_nearest_varying(_this,x0,y0);
  const vec3f c01 = Image3f__get_nearest_varying(_this,x0,y1);
  const vec3f c10 = Image3f__get_nearest_varying(_this,x1,y0);
  const vec3f c11 = Image3f__get_nearest_varying(_this,x1,y1);
  
  /* interpolate texels */
  return lerp(sy,
              lerp(sx,c00,c10),
              lerp(sx,c01,c11));
}

void Image3f__Destructor(uniform Image3f* uniform _this)
{
  uniform Image3f* uniform this = (uniform Image3f* uniform) _this;
  if (this->owned) delete[] this->data;
  Image__Destructor(&_this->base);
}

void Image3f__Constructor(uniform Image3f* uniform this, const uniform vec2ui size, 
						  uniform vec3f* uniform data, uniform bool owned)
{
  Image__Constructor(&this->base,
                     //Image3f__Destructor,
                     size,
                     Image3f__get_nearest_uniform,
                     Image3f__get_nearest_varying,
                     Image3f__get_bilinear_varying);
  this->data = data;
  this->owned = owned;
}

export void* uniform Image3f__new(const uniform int size_x, const uniform int size_y, uniform vec3f* uniform data, const uniform bool copy)
{
  uniform Image3f *uniform this = uniform new uniform Image3f;
  if (copy) {
    uniform vec3f* uniform local = uniform new uniform vec3f[size_x*size_y];
    memcpy(local,data,size_x*size_y*sizeof(uniform vec3f));
    Image3f__Constructor(this,make_vec2ui(size_x,size_y),local,copy);
  } else {
    Image3f__Constructor(this,make_vec2ui(size_x,size_y),data,copy);
  }
  return this;
}
