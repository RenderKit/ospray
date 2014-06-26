/*! \defgroup ospray_module_streamlines Experimental Direct Volume Rendering (STREAMLINES) Module

  \ingroup ospray_modules

  \brief Experimental direct volume rendering (STREAMLINES) functionality 

  Implements both scalar and ISPC-vectorized STREAMLINES renderers for various
  volume data layouts. Much of that functionality will eventually flow
  back into the ospray core (some already has); this module is mostly
  for experimental code (e.g., different variants of data structures
  or sample codes) that we do not (yet) want to have in the main
  ospray codebase (being a module allows this code to be conditionally
  enabled/disabled, and allows ospray to be built even if the streamlines
  module for some reason doesn't build).

*/

#pragma once

#include "ospray/geometry/geometry.h"

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \defgroup geometry_disks Disks ("disks") 

    \ingroup ospray_supported_geometries

    \brief Geometry representing sphers with a per-disk radius

    Implements a geometry consisinting of individual spherse, each of
    which can have a radius.  To allow a variety of disk
    representations this geomoetry allows a flexible way of specifying
    the offsets of origin, radius, and material ID within a data array
    of 32-bit floats.

    Parameters:
    <dl>
    <dt><code>float        radius = 0.f</code></dt><dd>Base radius common to all disks</dd>
    <dt><code>float        materialID = 0.f</code></dt><dd>Base radius common to all disks</dd>
    <dt><code>int32        offset_radius = -1</code></dt><dd>Offset of each disk's 'float radius' value within each disk. Setting this value to -1 means that there is no per-disk radius value, and that all disks should use the (shared) 'base_radius' value instead</dd>
    <dt><code>int32        bytes_per_disk = 4*sizeof(flaot)</code></dt><dd>Size (in bytes) of each disk in the data array.</dd>
    <dt><code>int32        offset_materialID = -1</code></dt><dd>Offset of each disk's 'int mateiralID' value within each disk. Setting this value to -1 means that there is no per-disk material ID, and that all disks share the same er-geomtetry materialID</dd>
    <dt><code>int32        center_offset = 0</code></dt><dd>Offset (in bytes) of each disk's 'vec3f center' value within the disk</dd>
    <dt><li><code>Data<float> disks</code></dt><dd> Array of data elements.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::DiskSet class.

  */



  /*! \brief A geometry for a set of disks

    Implements the \ref geometry_disks geometry

  */
  struct Disks : public Geometry {
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Disks"; }
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model);

    Ref<Data> vertexData;  //!< refcounted data array for vertex data
    Ref<Data> indexData; //!< refcounted data array for segment data

    float radius;   //!< default radius, if no per-disk radius was specified.
    int32 materialID;
    
    size_t numDisks;
    size_t bytesPerDisk; //!< num bytes per disk
    int64 offset_center;
    int64 offset_normal;
    int64 offset_radius;
    int64 offset_materialID;

    Ref<Data> data;
    
    Disks();
  };
}
/*! @} */

