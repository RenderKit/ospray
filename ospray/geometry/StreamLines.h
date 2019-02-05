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

namespace ospray {

  /*! @{ \ingroup ospray_module_streamlines */

  /*! \defgroup geometry_streamlines Stream Lines ("streamlines")

    \ingroup ospray_supported_geometries

    \brief Geometry representing stream lines of a fixed radius

    Implements a geometry consisting of stream lines, i.e.
    connected (and rounded) cylinder segments. Each
    stream line is specified by a set of vec3fa control points; all
    vertices belonging to to the same logical stream line are
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
    respectively, would internally correspond to 5 links (A-B, B-C,
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
    <dt><li><code>Data<vec3fa> color</code></dt><dd> Array of vertex colors corresponding to the vertices in this geometry.</dd>
    </dl>

    The functionality for this geometry is implemented via the
    \ref ospray::StreamLines class.

    \image html sl2.jpg

  */

  /*! \brief A geometry for stream line curves

    Implements the \ref geometry_streamlines geometry

  */
  struct OSPRAY_SDK_INTERFACE StreamLines : public Geometry
  {
    StreamLines();
    virtual ~StreamLines() override = default;
    virtual std::string toString() const override;
    virtual void finalize(Model *model) override;

    // Data members //

    Ref<Data> vertexData; //!< refcounted data array for vertex data
    Ref<Data> indexData; //!< refcounted data array for segment data
    Ref<Data> colorData; //!< refcounted data array for vertex color data
    Ref<Data> radiusData; //!< refcounted data array for vertex radius data

    const vec3fa *vertex {nullptr};
    size_t        numVertices {0};
    const uint32 *index {nullptr};
    size_t        numSegments {0};
    std::vector<vec4f> vertexCurve;
    std::vector<uint32> indexCurve;
  };
  /*! @} */

} // ::ospray

