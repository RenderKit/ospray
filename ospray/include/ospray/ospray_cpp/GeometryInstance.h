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
#include <ospray/ospray_cpp/Material.h>

namespace ospray {
  namespace cpp {

    class GeometryInstance : public ManagedObject_T<OSPGeometryInstance>
    {
     public:
      GeometryInstance(const Geometry &geom);
      GeometryInstance(OSPGeometry geom);

      GeometryInstance(const GeometryInstance &copy);
      GeometryInstance(OSPGeometryInstance existing);

      void setMaterial(Material &m) const;
      void setMaterial(OSPMaterial m) const;
    };

    // Inlined function definitions ///////////////////////////////////////////

    inline GeometryInstance::GeometryInstance(const Geometry &geom)
        : GeometryInstance(geom.handle())
    {
    }

    inline GeometryInstance::GeometryInstance(OSPGeometry existing)
    {
      ospObject = ospNewGeometryInstance(existing);
    }

    inline GeometryInstance::GeometryInstance(const GeometryInstance &copy)
        : ManagedObject_T<OSPGeometryInstance>(copy.handle())
    {
    }

    inline GeometryInstance::GeometryInstance(OSPGeometryInstance existing)
        : ManagedObject_T<OSPGeometryInstance>(existing)
    {
    }

    inline void GeometryInstance::setMaterial(Material &m) const
    {
      setMaterial(m.handle());
    }

    inline void GeometryInstance::setMaterial(OSPMaterial m) const
    {
      ospSetMaterial(handle(), m);
    }

  }  // namespace cpp
}  // namespace ospray
