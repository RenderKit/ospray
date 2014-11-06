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

  struct PKDGeometry : public Geometry {
    struct Particle {
      vec3f position;
      float attribute;
    };

    struct InnerNodeInfo {
      uint32 getBinBits() const { return bits & ((1<<30)-1); }
      uint32 getDim() const { return bits >> 30; }

      void setBinBits(uint32 binBits) { bits = binBits | (getDim() << 30); }
      void setDim(uint32 dim) { bits = getBinBits() | (dim << 30); }
    private:
      uint32 bits;
    };

    PKDGeometry();
    virtual ~PKDGeometry();

    Ref<Data> particleData;
    Ref<TransferFunction> transferFunction;

    Particle *particle;
    size_t numParticles;
    InnerNodeInfo *innerNodeInfo;
    size_t numInnerNodes;
    float radius;

    box3f bounds;
    float attr_lo, attr_hi;

    uint32 makeRangeBits(float attribute);
    virtual void finalize(Model *model);
    uint32 updateInnerNodeInfo(const box3f &bounds, size_t nodeID);

    /*! gets called whenever any of this node's dependencies got changed */
    virtual void dependencyGotChanged(ManagedObject *object);
  };

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

