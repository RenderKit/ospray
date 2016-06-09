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

#pragma once

#include <ospray_cpp/ManagedObject.h>
#include <ospray_cpp/Material.h>

namespace ospray {
namespace cpp    {

class Geometry : public ManagedObject_T<OSPGeometry>
{
public:

  Geometry(const std::string &type);
  Geometry(const Geometry &copy);
  Geometry(OSPGeometry existing);

  void setMaterial(Material &m);
  void setMaterial(OSPMaterial m);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Geometry::Geometry(const std::string &type)
{
  OSPGeometry c = ospNewGeometry(type.c_str());
  if (c) {
    m_object = c;
  } else {
    throw std::runtime_error("Failed to create OSPGeometry!");
  }
}

inline Geometry::Geometry(const Geometry &copy) :
  ManagedObject_T(copy.handle())
{
}

inline Geometry::Geometry(OSPGeometry existing) :
  ManagedObject_T(existing)
{
}

inline void Geometry::setMaterial(Material &m)
{
  setMaterial(m.handle());
}

inline void Geometry::setMaterial(OSPMaterial m)
{
  ospSetMaterial(handle(), m);
}

}// namespace cpp
}// namespace ospray
