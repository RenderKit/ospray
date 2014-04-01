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

  /*! \brief A geometry for stream line curves

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
    <ul>
    <li><code>float        radius</code> Radius to be used for all stream lines</li>
    <li><code>Data<vec3fa> vertex</code> Vertices of stream line segments</li>
    <li><code>Data<int32>  index </code> Start indices of each segment</li>
    </ul>
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
  };
}
/*! @} */

