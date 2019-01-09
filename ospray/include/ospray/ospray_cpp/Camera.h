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

class Camera : public ManagedObject_T<OSPCamera>
{
public:

  Camera(const std::string &type);
  Camera(const Camera &copy);
  Camera(OSPCamera existing = nullptr);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Camera::Camera(const std::string &type)
{
  OSPCamera c = ospNewCamera(type.c_str());
  if (c) {
    ospObject = c;
  } else {
    throw std::runtime_error("Failed to create OSPCamera!");
  }
}

inline Camera::Camera(const Camera &copy) :
  ManagedObject_T<OSPCamera>(copy.handle())
{
}

inline Camera::Camera(OSPCamera existing) :
  ManagedObject_T<OSPCamera>(existing)
{
}

}// namespace cpp
}// namespace ospray
