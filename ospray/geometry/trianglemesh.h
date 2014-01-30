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

    //! helper fct that creates a tessllated unit arrow
    /*! this function creates a tessllated 'unit' arrow, where 'unit'
        means itreaches from Z=-1 to Z=+1. With of arrow head and
        arrow body are given as parameters, as it the number of
        segments for tessellating. The total number of triangles from
        this arrow is 'numSegments*5'. */
  ospray::TriangleMesh *makeArrow(int numSegments=64,
                                  float headWidth=.5f,
                                  float bodyWidth=.25f,
                                  float headLength=1.f);
};
