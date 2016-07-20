// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// ospray stuff
#include "geometry/Geometry.h"
#include "volume/Volume.h"

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
    typedef std::vector<Ref<Volume> > VolumeVector;
    
    //! \brief vector of all geometries used in this model
    GeometryVector geometry;
    //! \brief vector of all volumes used in this model
    VolumeVector volume;

// #if EXP_DATA_PARALLEL
//     /*! list of all pieces of data parallel volumes, including those
//         that we do NOT own. note that some of these pieces may be
//         owned by multiple owners */
//     std::vector<Ref<Volume::DataParallelPiece> > dpVolumePieces;
// #endif

    //! \brief the embree scene handle for this geometry
    RTCScene embreeSceneHandle; 
    box3f bounds;
  };

} // ::ospray
