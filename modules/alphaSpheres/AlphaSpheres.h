/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// ospray core
#include "ospray/geometry/geometry.h"
#include "ospray/transferfunction/TransferFunction.h"
// our bvh
#include "MinMaxBVH2.h"

/*! @{ \ingroup ospray_module_streamlines */
namespace ospray {

  /*! \brief A geometry for a set of alpha-(and color-)mapped spheres

    Implements the \ref geometry_spheres geometry

  */
  struct AlphaSpheres : public Geometry {

    struct PrimAbstraction : public MinMaxBVH::PrimAbstraction {
      AlphaSpheres *as;

      PrimAbstraction(AlphaSpheres *as) : as(as) {};

      virtual size_t numPrims() { return as->numSpheres; }
      // virtual size_t sizeOfPrim() { return sizeof(vec3f); }
      // virtual void swapPrims(size_t aID, size_t bID) 
      // { std::swap(sphere[aID],sphere[bID]); }
      virtual float attributeOf(size_t primID) 
      { return as->attribute[primID]; }
      
      virtual box3f boundsOf(size_t primID) 
      { 
        box3f b;
        b.lower = as->position[primID] - vec3f(as->radius);
        b.upper = as->position[primID] + vec3f(as->radius);
        return b;
      }
    };


    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::AlphaSpheres"; }
    /*! \brief integrates this geometry's primitives into the respective
      model's acceleration structure */
    virtual void finalize(Model *model);
    
    Ref<Data> positionData;  //!< refcounted data array for vertex data
    Ref<Data> attributeData;  //!< refcounted data array for vertex data
    vec3f    *position;
    float    *attribute;

    float radius;
    
    size_t numSpheres;

    MinMaxBVH mmBVH;

    void buildBVH();

    Ref<TransferFunction> transferFunction;

    AlphaSpheres();
  };
}
/*! @} */

