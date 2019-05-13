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

#include "common/Data.h"
#include "Geometry.h"

namespace ospray {

  struct OSPRAY_SDK_INTERFACE GeometryInstance : public ManagedObject
  {
    GeometryInstance(Geometry *geometry);
    virtual ~GeometryInstance() override;
    virtual std::string toString() const override;

    virtual void setMaterial(Material *mat);
    virtual void setMaterialList(Data *matListData);

    virtual void commit() override;

    RTCGeometry embreeGeometryHandle() const;

    box3f bounds() const;

    AffineSpace3f xfm() const;

   private:
    // Data members //

    // Geometry information
    box3f instanceBounds;
    AffineSpace3f instanceXfm;
    Ref<Geometry> instancedGeometry;

    // Embree information
    RTCScene embreeSceneHandle{nullptr};
    RTCGeometry embreeInstanceGeometry{nullptr};
    RTCGeometry lastEmbreeInstanceGeometryHandle{nullptr}; // to detect updates
    int embreeID {-1};

    // Appearance information
    Ref<Data> prim_materialIDData; /*!< data array for per-prim material ID
                                      (uint32) */
    Ref<Data> colorData;
    Material **materialList{nullptr};  //!< per-primitive material list
    Ref<Data> materialListData;  //!< data array for per-prim materials
    std::vector<void *>
        ispcMaterialPtrs;  //!< pointers to ISPC equivalent materials

    friend struct PathTracer; // TODO: fix this!
  };

}  // namespace ospray
