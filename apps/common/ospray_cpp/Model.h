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

#include <ospray_cpp/Geometry.h>
#include <ospray_cpp/ManagedObject.h>
#include <ospray_cpp/Volume.h>

namespace ospray {
namespace cpp    {

class Model : public ManagedObject_T<OSPModel>
{
public:

  Model();
  Model(const Model &copy);
  Model(OSPModel existing);

  void addGeometry(Geometry &v);
  void addGeometry(OSPGeometry v);

  void removeGeometry(Geometry &v);
  void removeGeometry(OSPGeometry v);

  void addVolume(Volume &v);
  void addVolume(OSPVolume v);
};

// Inlined function definitions ///////////////////////////////////////////////

inline Model::Model()
{
  OSPModel c = ospNewModel();
  if (c) {
    m_object = c;
  } else {
    throw std::runtime_error("Failed to create OSPModel!");
  }
}

inline Model::Model(const Model &copy) :
  ManagedObject_T(copy.handle())
{
}

inline Model::Model(OSPModel existing) :
  ManagedObject_T(existing)
{
}

inline void Model::addGeometry(Geometry &v)
{
  addGeometry(v.handle());
}

inline void Model::addGeometry(OSPGeometry v)
{
  ospAddGeometry(handle(), v);
}

inline void Model::removeGeometry(Geometry &v)
{
  removeGeometry(v.handle());
}

inline void Model::removeGeometry(OSPGeometry v)
{
  ospRemoveGeometry(handle(), v);
}

inline void Model::addVolume(Volume &v)
{
  addVolume(v.handle());
}

inline void Model::addVolume(OSPVolume v)
{
  ospAddVolume(handle(), v);
}

}// namespace cpp
}// namespace ospray
