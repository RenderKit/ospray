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

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \defgroup geometry_cylinders Cylinders ("cylinders")

    \ingroup ospray_supported_geometries

    \brief Geometry representing cylinders with a per-cylinder radius

    Implements a geometry consisting of individual cylinders, each
    of which can have a radius.  To allow a variety of cylinder
    representations this geometry allows a flexible way of specifying
    the offsets of start, end, radius, and material ID within a data array
    of 32-bit floats.

    Parameters:
    <dl>
    <dt><code>float        radius = 0.01f</code></dt><dd>Base radius common to all cylinders if 'offset_radius' is not used</dd>
    <dt><code>int32        materialID = 0</code></dt><dd>Material ID common to all cylinders if 'offset_materialID' is not used</dd>
    <dt><code>int32        bytes_per_cylinder = 6*sizeof(float)</code></dt><dd>Size (in bytes, default is for v0/v1 positions) of each cylinder in the data array.</dd>
    <dt><code>int32        offset_v0 = 0</code></dt><dd>Offset (in bytes) of each cylinder's 'vec3f v0' value (the start vertex) within each cylinder</dd>
    <dt><code>int32        offset_v1 = 3*sizeof(float)</code></dt><dd>Offset (in bytes) of each cylinder's 'vec3f v1' value (the end vertex) within each cylinder</dd>
    <dt><code>int32        offset_radius = -1</code></dt><dd>Offset (in bytes) of each cylinder's 'float radius' value within each cylinder. Setting this value to -1 means that there is no per-cylinder radius value, and that all cylinders should use the (shared) 'radius' value instead</dd>
    <dt><code>int32        offset_materialID = -1</code></dt><dd>Offset (in bytes) of each cylinder's 'int materialID' value within each cylinder. Setting this value to -1 means that there is no per-cylinder material ID, and that all cylinders share the same per-geometry 'materialID'</dd>
    <dt><code>int32        offset_colorID = -1</code></dt><dd>Byte offset for each cylinder's color index (for the 'color' data). Setting this value to -1 means that there is no per-cylinder color, and that all cylinders share the same per-geometry color.</dd>
    <dt><code>Data<float>  cylinders</code></dt><dd>Array of data elements.</dd>
    <dt><code>Data<float>  color</code></dt><dd>Array of color (RGBA) elements accessed by indexes (per element) in 'cylinders' colorID data.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::Cylinders class.

  */

  /*! \brief A geometry for a set of cylinders

    Implements the \ref geometry_cylinders geometry

  */
  struct OSPRAY_SDK_INTERFACE Cylinders : public Geometry
  {
    Cylinders();
    //! \brief common function to help printf-debugging
    virtual std::string toString() const override;
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model) override;

    float radius;   //!< default radius, if no per-cylinder radius was specified.
    int32 materialID;

    size_t numCylinders;
    size_t bytesPerCylinder; //!< num bytes per cylinder
    int64 offset_v0;
    int64 offset_v1;
    int64 offset_radius;
    int64 offset_materialID;
    int64 offset_colorID;

    Ref<Data> cylinderData;
    Ref<Data> colorData; /*!< cylinder color array */
    Ref<Data> texcoordData;
  };
/*! @} */

} // ::ospray
