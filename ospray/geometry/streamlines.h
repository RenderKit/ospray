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

#include "geometry.h"

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \defgroup geometry_streamlines Stream Lines ("streamlines") 

    \ingroup ospray_supported_geometries

    \brief Geometry representing stream lines of a fixed radius

    \image html sl2.jpg

    Implements a geometry consisinting of stream lines
    consisting of connected (and rounded) cylinder segments. Each
    stream line is specified by a set of vec3fa control points; all
    vertices belonging to to the same logiical stream line are
    connected via cylinders of a fixed radius, with additional spheres
    at each vertex to make for a smooth transition between the
    cylinders. A "streamline" geometry can contain multiple disjoint
    stream lines; all such stream lines use the same radius that is
    specified for this geometry.

    A stream line geometry is created via \ref ospNewGeometry with
     type string "streamlines".

    Each streamline is specified as a list of linear segments, where
    each segment is pretty much a cylinder with rounded ends. The
    "streamline" geometry is built over those links (not over the
    curves), so the input to the geometry object are a) a list of
    vertices, plus b) a list of ints, where each int specifies the
    first vertex of a link (the second one being "index+1").

    For example, two streamlines of vertices (A-B-C-D) and (E-F-G),
    respectively, would internally correspond to 7 links (A-B, B-C,
    C-D, E-F, and F-G), and could be specified via an array of
    vertices "A,B,C,D,E,F,G", plus an array of link offsets
    "0,1,2,4,5"

    Vertices are stored in vec3fa form; the user is free to use the
    'w' value of each vertex to store additional value (such as curve
    ID, or an attribute to be visualized); ospray will not touch this
    value

    Parameters:
    <dl>
    <dt><code>float        radius</code></dt><dd> Radius to be used for all stream lines</dd>
    <dt><li><code>Data<vec3fa> vertex</code></dt><dd> Array of all vertices for *all* curves in this geometry, one curve's vertices stored after another.</dd>
    <dt><li><code>Data<int32>  index </code></dt><dd> index[i] specifies the index of the first vertex of the i'th curve. The curve then uses all following vertices in the 'vertex' array until either the next curve starts, or the array's end is reached.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::TriangleMesh class.
  */



  /*! \brief A geometry for stream line curves

    Implements the \ref geometry_streamlines geometry

  */
  struct StreamLines : public Geometry {
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::StreamLines"; }
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model);

    Ref<Data> vertexData;  //!< refcounted data array for vertex data
    Ref<Data> indexData; //!< refcounted data array for segment data

    const vec3fa *vertex;
    size_t        numVertices;
    const uint32 *index;
    size_t        numSegments;
    float         radius;

    StreamLines();
  };
}
/*! @} */

