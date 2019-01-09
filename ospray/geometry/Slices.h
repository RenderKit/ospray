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

  /*! \defgroup geometry_slices Slices ("slices")

    \ingroup ospray_supported_geometries

    \brief Geometry representing slices of a volume

    Implements a geometry consisting of slices of a volume. Each
    slice is a plane that is colored according to a provided volume's
    transfer function.

    Parameters:
    <dl>
    <dt><li><code>Data<vec4f> planes</code></dt><dd> Array of planes for all slices in this geometry. The vec4f for each plane consists of the (a,b,c,d) coefficients of the plane equation a*x + b*y + c*z + d = 0.</dd>
    <dt><li><code>Volume  volume </code></dt><dd> volume specifies the volume to be slices. The color of the slice will be mapped through the volume's transfer function.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::Slices class.

  */

  /*! \brief A geometry for slices of volumes

    Implements the \ref geometry_slices geometry

  */
  struct OSPRAY_SDK_INTERFACE Slices : public Geometry
  {
    Slices();
    virtual ~Slices() override = default;
    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    // Data members //

    Ref<Data> planesData; //!< refcounted data array for planes data
    Ref<Volume> volume;

    size_t numPlanes;
    vec4f *planes;
  };
  /*! @} */

} // ::ospray
