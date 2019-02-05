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
#include "volume/Volume.h"

namespace ospray {

  /*! \defgroup geometry_isosurfaces Isosurfaces ("isosurfaces")

    \ingroup ospray_supported_geometries

    \brief Geometry representing isosurfaces of a volume

    Implements a geometry consisting of isosurfaces of a volume. Each
    isosurface is colored according to a provided volume's transfer
    function.

    Parameters:
    <dl>
    <dt><li><code>Data<float> isovalues</code></dt><dd> Array of floats for all isovalues in this geometry.</dd>
    <dt><li><code>Volume  volume </code></dt><dd> volume specifies the volume to be isosurfaced. The color of the isosurface(s) will be mapped through the volume's transfer function.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::Isosurfaces class.

  */

  /*! \brief A geometry for isosurfaces of volumes

    Implements the \ref geometry_isosurfaces geometry

  */
  struct OSPRAY_SDK_INTERFACE Isosurfaces : public Geometry
  {
    Isosurfaces();
    virtual ~Isosurfaces() override = default;

    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    // Data members //

    Ref<Data> isovaluesData; //!< refcounted data array for isovalues data
    Ref<Volume> volume;

    size_t numIsovalues;
    float *isovalues;
  };
  /*! @} */

} // ::ospray
