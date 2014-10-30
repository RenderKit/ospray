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
    //! note: currently MUST be a multiple 8 b in size
    struct Sphere {
      vec3f position;
      float radius;
      float attribute;
      int typeID;
    };

    struct PrimAbstraction : public MinMaxBVH::PrimAbstraction {
      Sphere *sphere;
      size_t numSpheres;
      PrimAbstraction(Sphere *sphere, size_t numSpheres)
        : sphere(sphere), numSpheres(numSpheres) 
      {}

      virtual size_t numPrims() { return numSpheres; }
      virtual size_t sizeOfPrim() { return sizeof(*sphere); }
      virtual void swapPrims(size_t aID, size_t bID) 
      { std::swap(sphere[aID],sphere[bID]); }
      virtual box4f boundsOfPrim(size_t i) { 
        box4f b;
        (vec3f&)b.lower = sphere[i].position - vec3f(sphere[i].radius);
        (vec3f&)b.upper = sphere[i].position + vec3f(sphere[i].radius);
        b.lower.w = b.upper.w = sphere[i].attribute;
        return b;
      }
    };

    //! \brief common function to help printf-debugging 
    virtual std::string toString() const { return "ospray::AlphaSpheres"; }
    /*! \brief integrates this geometry's primitives into the respective
      model's acceleration structure */
    virtual void finalize(Model *model);
    
    Ref<Data> sphereData;  //!< refcounted data array for vertex data
    Sphere   *sphere;

    // float radius;   //!< default radius, if no per-sphere radius was specified.
    // int32 materialID;
    
    size_t numSpheres;
    size_t bytesPerSphere; //!< num bytes per sphere
    int64 offset_center;
    int64 offset_radius;
    int64 offset_attribute;

    MinMaxBVH mmBVH;
    void buildBVH();

    //    Ref<Data> data;
    // Ref<Data> materialList;
    // void     *_materialList;
    Ref<TransferFunction> transferFunction;

    AlphaSpheres();
  };
}
/*! @} */

