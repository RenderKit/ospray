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

#include <ospray/ospray_cpp/Geometry.h>
#include <ospray/ospray_cpp/ManagedObject.h>
#include <ospray/ospray_cpp/Volume.h>

namespace ospray {
  namespace cpp    {

    class World : public ManagedObject_T<OSPWorld>
    {
    public:

      World();
      World(const World &copy);
      World(OSPWorld existing);

      void addGeometry(Geometry &v) const;
      void addGeometry(OSPGeometry v) const;

      void removeGeometry(Geometry &v) const;
      void removeGeometry(OSPGeometry v) const;

      void addVolume(Volume &v) const;
      void addVolume(OSPVolume v) const;

      void removeVolume(Volume &v) const;
      void removeVolume(OSPVolume v) const;

      Geometry createInstance(const ospcommon::affine3f &transform) const;
    };

    // Inlined function definitions ///////////////////////////////////////////////

    inline World::World()
    {
      OSPWorld c = ospNewWorld();
      if (c) {
        ospObject = c;
      } else {
        throw std::runtime_error("Failed to create OSPWorld!");
      }
    }

    inline World::World(const World &copy) :
      ManagedObject_T<OSPWorld>(copy.handle())
    {
    }

    inline World::World(OSPWorld existing) :
      ManagedObject_T<OSPWorld>(existing)
    {
    }

    inline void World::addGeometry(Geometry &v) const
    {
      addGeometry(v.handle());
    }

    inline void World::addGeometry(OSPGeometry v) const
    {
      ospAddGeometry(handle(), v);
    }

    inline void World::removeGeometry(Geometry &v) const
    {
      removeGeometry(v.handle());
    }

    inline void World::removeGeometry(OSPGeometry v) const
    {
      ospRemoveGeometry(handle(), v);
    }

    inline void World::addVolume(Volume &v) const
    {
      addVolume(v.handle());
    }

    inline void World::addVolume(OSPVolume v) const
    {
      ospAddVolume(handle(), v);
    }

    inline void World::removeVolume(Volume &v) const
    {
      removeVolume(v.handle());
    }

    inline void World::removeVolume(OSPVolume v) const
    {
      ospRemoveVolume(handle(), v);
    }

    inline Geometry
    World::createInstance(const ospcommon::affine3f &transform) const
    {
      return ospNewInstance(handle(), (const osp_affine3f&)transform);
    }

  }// namespace cpp
}// namespace ospray
