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

#pragma once

#include <ospray/ospray_cpp/ManagedObject.h>

namespace ospray {
namespace cpp    {

class Texture : public ManagedObject_T<OSPTexture>
{
public:

  Texture(const Texture &copy);
  Texture(OSPTexture existing);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Texture::Texture(const Texture &copy) :
  ManagedObject_T<OSPTexture>(copy.handle())
{
}

inline Texture::Texture(OSPTexture existing) :
  ManagedObject_T<OSPTexture>(existing)
{
}

}// namespace cpp
}// namespace ospray
