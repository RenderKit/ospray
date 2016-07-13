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

#include "Geometry.h"
#include "common/Data.h"

namespace ospray {

  /*! \defgroup geometry_instance Instancing ("instance") 

    \brief Implements instancing via a single instance of another
    model.

    \ingroup ospray_supported_geometries

    Once created, a trianglemesh recognizes the following parameters
    <pre>
    float3 "xfm.l.vx" // 1st column of the affine transformation matrix
    float3 "xfm.l.vy" // 1st column of the affine transformation matrix
    float3 "xfm.l.vz" // 1st column of the affine transformation matrix
    float3 "xfm.p"    // 4th column (translation) of the affine transformation matrix
    OSPModel "model"  // model we're instancing
    </pre>

    The functionality for this geometry is implemented via the
    \ref ospray::Instance class.
  */

  typedef affine3f AffineSpace3f;

  /*! \brief A Single Instance

   */
  struct Instance : public Geometry
  {
    /*! Constructor */
    Instance();
    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::Instance"; }
    /*! \brief integrates this geometry's primitives into the respective
        model's acceleration structure */
    virtual void finalize(Model *model);

    /*! transformation matrix associated with that instance's geometry. may be embree::one */
    AffineSpace3f xfm;
    /*! reference to instanced model. Must be a *model* that we're instancing, not a geometry */
    Ref<Model>    instancedScene;
    /*! geometry ID of this geometry in the parent model */
    uint32        embreeGeomID; 
  };

} // ::ospray
