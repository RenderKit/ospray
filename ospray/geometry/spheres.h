/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */



#pragma once

#include "geometry.h"

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \defgroup geometry_spheres Spheres ("spheres") 

    \ingroup ospray_supported_geometries

    \brief Geometry representing sphers with a per-sphere radius

    Implements a geometry consisinting of individual spherse, each of
    which can have a radius.  To allow a variety of sphere
    representations this geomoetry allows a flexible way of specifying
    the offsets of origin, radius, and material ID within a data array
    of 32-bit floats.

    Parameters:
    <dl>
    <dt><code>float        radius = 0.f</code></dt><dd>Base radius common to all spheres</dd>
    <dt><code>float        materialID = 0.f</code></dt><dd>Base radius common to all spheres</dd>
    <dt><code>int32        offset_radius = -1</code></dt><dd>Offset of each sphere's 'float radius' value within each sphere. Setting this value to -1 means that there is no per-sphere radius value, and that all spheres should use the (shared) 'base_radius' value instead</dd>
    <dt><code>int32        bytes_per_sphere = 4*sizeof(flaot)</code></dt><dd>Size (in bytes) of each sphere in the data array.</dd>
    <dt><code>int32        offset_materialID = -1</code></dt><dd>Offset of each sphere's 'int mateiralID' value within each sphere. Setting this value to -1 means that there is no per-sphere material ID, and that all spheres share the same er-geomtetry materialID</dd>
    <dt><code>int32        center_offset = 0</code></dt><dd>Offset (in bytes) of each sphere's 'vec3f center' value within the sphere</dd>
    <dt><li><code>Data<float> spheres</code></dt><dd> Array of data elements.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::SphereSet class.

  */



  /*! \brief A geometry for a set of spheres

    Implements the \ref geometry_spheres geometry

  */
  struct Spheres : public Geometry {
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Spheres"; }
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model);

    Ref<Data> vertexData;  //!< refcounted data array for vertex data
    Ref<Data> indexData; //!< refcounted data array for segment data

    float radius;   //!< default radius, if no per-sphere radius was specified.
    int32 materialID;
    
    size_t numSpheres;
    size_t bytesPerSphere; //!< num bytes per sphere
    int64 offset_center;
    int64 offset_radius;
    int64 offset_materialID;

    Ref<Data> data;
    
    Spheres();
  };
}
/*! @} */

