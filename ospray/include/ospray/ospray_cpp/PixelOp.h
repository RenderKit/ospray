// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

class PixelOp : public ManagedObject_T<OSPPixelOp>
{
public:

  PixelOp() = default;
  PixelOp(const std::string &type);
  PixelOp(const PixelOp &copy);
  PixelOp(OSPPixelOp existing);
};

// Inlined function definitions ///////////////////////////////////////////////

inline PixelOp::PixelOp(const std::string &type)
{
  OSPPixelOp c = ospNewPixelOp(type.c_str());
  if (c) {
    ospObject = c;
  } else {
    throw std::runtime_error("Failed to create OSPPixelOp!");
  }
}

inline PixelOp::PixelOp(const PixelOp &copy) :
  ManagedObject_T<OSPPixelOp>(copy.handle())
{
}

inline PixelOp::PixelOp(OSPPixelOp existing) :
  ManagedObject_T<OSPPixelOp>(existing)
{
}


}// namespace cpp
}// namespace ospray
