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

#include "Geometry.h"
#include "ospray/OSPDataType.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE Spheres : public Geometry
  {
    Spheres();
    virtual ~Spheres() override = default;

    virtual std::string toString() const override;

    virtual void commit() override;

    virtual size_t numPrimitives() const override;

   protected:
    /*! default radius, if no per-sphere radius was specified. */
    float radius;
    int32 materialID;

    size_t numSpheres;
    size_t bytesPerSphere;  //!< num bytes per sphere
    int64 offset_center;
    int64 offset_radius;

    Ref<Data> sphereData;
    Ref<Data> texcoordData;

    /*! The color format of the colorData array, one of:
        OSP_VEC3F, OSP_VEC3FA, OSP_VEC4F or OSP_UCHAR4 */
    OSPDataType colorFormat;

    bool huge_mesh{false};

   private:
    void createEmbreeGeometry() override;
  };
  /*! @} */

}  // namespace ospray
