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

#include "vec.h"

namespace ospcommon {
  // -------------------------------------------------------
  // parsing from strings
  // -------------------------------------------------------
  vec2f toVec2f(const char *ptr)
  {
    assert(ptr);
    vec2f v;
    int rc = sscanf(ptr,"%f %f",&v.x,&v.y); 
    assert(rc == 2);
    return v;
  }

  vec3f toVec3f(const char *ptr)
  {
    assert(ptr);
    vec3f v;
    int rc = sscanf(ptr,"%f %f %f",&v.x,&v.y,&v.z); 
    assert(rc == 3);
    return v;
  }

  vec4f toVec4f(const char *ptr)
  {
    assert(ptr);
    vec4f v;
    int rc = sscanf(ptr,"%f %f %f %f",&v.x,&v.y,&v.z,&v.w); 
    assert(rc == 4);
    return v;
  }

  vec2i toVec2i(const char *ptr)
  {
    assert(ptr);
    vec2i v;
    int rc = sscanf(ptr,"%i %i",&v.x,&v.y); 
    assert(rc == 2);
    return v;
  }

  vec3i toVec3i(const char *ptr)
  {
    assert(ptr);
    vec3i v;
    int rc = sscanf(ptr,"%i %i %i",&v.x,&v.y,&v.z); 
    assert(rc == 3);
    return v;
  }

  vec4i toVec4i(const char *ptr)
  {
    assert(ptr);
    vec4i v;
    int rc = sscanf(ptr,"%i %i %i %i",&v.x,&v.y,&v.z,&v.w); 
    assert(rc == 4);
    return v;
  }

} // ::ospcommon
