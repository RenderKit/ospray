/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// viewer
#include "ospray/common/OspCommon.h"

namespace ospray {
  namespace viewer {

    //! \brief Helper class for storing tessellated geometry (with
    //! single color per mesh); that can be used to store 3D geometry
    //! for 3D widgets
    struct HelperGeometry {
      //! a quad made up of four vertices with given vertex normal
      struct Quad {
        vec3f vtx[4], nor[4];
      };
      //! a mesh/indexed face set of triangles
      struct Mesh {
        //! base color for this mesh; shared among all vertices
        vec3f color;
        //! vertex array
        std::vector<vec3fa> vertex;
        //! normal array
        std::vector<vec3fa> normal;
        //! triangle/index array
        std::vector<vec3i>  index;
        
        //! \brief add a new quad with given vertices to this triangle mesh
        void addQuad(const affine3f &xfm, const Quad &quad);
      };
    };

    //! \brief A triangle mesh representing a coordinate frame with
    //! three tessellated arrows, with first pointing in X direction
    //! (colored red), the second in Y (colored greed), the third in Z
    //! (colored blue). */
    struct CoordFrameGeometry : public HelperGeometry {
      Mesh arrow[3];
      float shaftThickness;
      float headLength;
      int numSegments;
      
      CoordFrameGeometry();
      void makeArrow(Mesh &mesh,const vec3f &axis);
    };
  } // ::viewer
} // ::ospray

