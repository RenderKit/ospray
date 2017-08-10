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

#include "common.ih"

uniform int getVec4iComponent(uniform vec4i v, varying int i)
{
  if (i == 0) {
    return v.x;
  }

  if (i == 1) {
    return v.y;
  }

  if (i == 2) {
    return v.z;
  }

  if (i == 3) {
    return v.w;
  }

  return 0;
}
