#pragma once

#include "geometry.h"
#include "../common/data.h"

namespace ospray {

  struct TriangleMesh : public Geometry
  {

    TriangleMesh();
    virtual std::string toString() const { return "ospray::TriangleMesh"; }
    virtual void finalize(Model *model);

    Ref<Data> idxData; /*!< triangle indices (A,B,C,materialID) */
    Ref<Data> posData; /*!< vertex position (vec3fa) */
    uint32    eMesh;   /*!< embree triangle mesh handle */

  };

};
