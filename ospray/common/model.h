/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray stuff
#include "../geometry/geometry.h"
// stl stuff
#include <vector>
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"

namespace ospray {

  /*! \brief Base Abstraction for an OSPRay 'Model' entity

    A 'model' is the generalization of a 'scene' in embree: it is a
    collection of geometries and volumes that one can trace rays
    against, and that one can afterwards 'query' for certain
    properties (like the shading normal or material for a given
    ray/model intersection) */
  struct Model : public ManagedObject
  {
    Model();

    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Model"; }
    virtual void finalize();

    typedef std::vector<Ref<Geometry> > GeometryVector;
    GeometryVector geometry;

    //! \brief the embree scene handle for this geometry
    RTCScene embreeSceneHandle; 
  };

};
