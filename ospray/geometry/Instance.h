#pragma once

#include "Geometry.h"
#include "ospray/common/Data.h"

namespace ospray {

  /*! \defgroup geometry_instance Instancing ("instance") 

    \brief Implements instancing via a single instnace of another
    model

    \ingroup ospray_supported_geometries

    Once created, a trianglemesh recognizes the following parameters
    <pre>
    affine3f "xfm"   // transformation matrix the model is instantiated with
    OSPModel "model" // model we're instancing
    </pre>

    The functionality for this geometry is implemented via the
    \ref ospray::Instance class.
  */

  typedef affine3f AffineSpace3f;

  /*! \brief A Single Instance

   */
  struct Instance : public Geometry
  {
    Instance();
    virtual std::string toString() const { return "ospray::Instance"; }
    virtual void finalize(Model *model);

    AffineSpace3f   xfm;
    Ref<Model> instancedScene;
    uint32     embreeGeomID; 
  };

} // ::ospray
