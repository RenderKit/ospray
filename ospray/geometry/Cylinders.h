/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



#pragma once

#include "Geometry.h"

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \defgroup geometry_cylinders Cylinders ("cylinders") 

    \ingroup ospray_supported_geometries

    \brief Geometry representing sphers with a per-cylinder radius

    Implements a geometry consisinting of individual spherse, each of
    which can have a radius.  To allow a variety of cylinder
    representations this geomoetry allows a flexible way of specifying
    the offsets of origin, radius, and material ID within a data array
    of 32-bit floats.

    Parameters:
    <dl>
    <dt><code>float        radius = 0.f</code></dt><dd>Base radius common to all cylinders</dd>
    <dt><code>float        materialID = 0.f</code></dt><dd>Base radius common to all cylinders</dd>
    <dt><code>int32        offset_radius = -1</code></dt><dd>Offset of each cylinder's 'float radius' value within each cylinder. Setting this value to -1 means that there is no per-cylinder radius value, and that all cylinders should use the (shared) 'base_radius' value instead</dd>
    <dt><code>int32        bytes_per_cylinder = 4*sizeof(flaot)</code></dt><dd>Size (in bytes) of each cylinder in the data array.</dd>
    <dt><code>int32        offset_v0 = -1</code></dt><dd>Offset of each cylinder's 'float radius' value within each cylinder. Setting this value to -1 means that there is no per-cylinder radius value, and that all cylinders should use the (shared) 'base_radius' value instead</dd>
    <dt><code>int32        offset_v1 = -1</code></dt><dd>Offset of each cylinder's 'float radius' value within each cylinder. Setting this value to -1 means that there is no per-cylinder radius value, and that all cylinders should use the (shared) 'base_radius' value instead</dd>
    <dt><li><code>Data<float> data</code></dt><dd> Array of data elements.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::CylinderSet class.

  */

  /*! \brief A geometry for a set of cylinders

    Implements the \ref geometry_cylinders geometry

  */
  struct Cylinders : public Geometry {
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Cylinders"; }
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model);

    Ref<Data> vertexData;  //!< refcounted data array for vertex data
    Ref<Data> indexData; //!< refcounted data array for segment data

    float radius;   //!< default radius, if no per-cylinder radius was specified.
    int32 materialID;
    
    size_t numCylinders;
    size_t bytesPerCylinder; //!< num bytes per cylinder
    int64 offset_v0;
    int64 offset_v1;
    int64 offset_radius;
    int64 offset_materialID;

    Ref<Data> data;
    
    Cylinders();
  };
}
/*! @} */

