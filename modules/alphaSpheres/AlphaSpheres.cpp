/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// ospray
#include "AlphaSpheres.h"
#include "ospray/common/Data.h"
#include "ospray/common/Model.h"
// ispc-generated files
#include "AlphaSpheres_ispc.h"
//#include "PartiKD.h"

namespace ospray {
  using std::cout;
  using std::endl;

  AlphaSpheres::AlphaSpheres()
  {
    this->ispcEquivalent = ispc::AlphaSpheres_create(this);
  }
  PKDGeometry::PKDGeometry()
  {
    this->ispcEquivalent = ispc::PKDGeometry_create(this);
  }
  
  void AlphaSpheres::buildBVH() 
  {
    PrimAbstraction pa(this);
    mmBVH.initialBuild(&pa);
  }
  
  void AlphaSpheres::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    attributeData     = getParamData("attributes",NULL);
    positionData      = getParamData("positions",NULL);
    transferFunction  = (TransferFunction *)getParamObject("transferFunction",NULL);
    
    if (positionData == NULL) 
      throw std::runtime_error("#osp:AlphaPositions: no 'positions' data specified");
    if (attributeData == NULL) 
      throw std::runtime_error("#osp:AlphaAttributes: no 'attributes' data specified");
    numSpheres = positionData->numBytes / sizeof(vec3f);

    std::cout << "#osp: creating 'alpha_spheres' geometry, #spheres = " << numSpheres << std::endl;
    
    if (numSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::Spheres: too many spheres in this sphere geometry. Consider splitting this geometry in multiple geometries with fewer spheres (you can still put all those geometries into a single model, but you can't put that many spheres into a single geometry without causing address overflows)");
    }
    assert(transferFunction);
    
    position  = (vec3f *)positionData->data;
    attribute = (float *)attributeData->data;

    buildBVH();

    ispc::AlphaSpheres_set(getIE(),
                           model->getIE(),
                           transferFunction->getIE(),
                           mmBVH.rootRef,
                           mmBVH.getNodePtr(),
                           &mmBVH.primID[0],
                           radius,
                           (ispc::vec3f*)position,attribute,
                           numSpheres);
    PRINT(mmBVH.node.size());
    PRINT(mmBVH.node[0]);
  }

  void PKDGeometry::finalize(Model *model)
  {
    radius            = getParam1f("radius",0.01f);
    particleData      = getParamData("particles",NULL);
    transferFunction  = (TransferFunction *)getParamObject("transferFunction",NULL);
    
    if (particleData == NULL) 
      throw std::runtime_error("#osp:AlphaPositions: no 'particles' data specified");
    numParticles = particleData->numBytes / sizeof(Particle);

    std::cout << "#osp: creating 'pkd_geometry' geometry, #spheres = " << numParticles << std::endl;
    
    if (numParticles >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::PKDGeometry: too many particles in this sphere geometry. Consider splitting this geometry in multiple geometries with fewer particles (you can still put all those geometries into a single model, but you can't put that many spheres into a single geometry without causing address overflows)");
    }
    assert(transferFunction);
    
    particle  = (Particle *)particleData->data;
    numInnerNodes = numParticles / 2;
    innerNodeInfo = new InnerNodeInfo[numInnerNodes];

    bounds = embree::empty;
    attr_lo = std::numeric_limits<float>::infinity();
    attr_hi = -std::numeric_limits<float>::infinity();
    for (int i=0;i<numParticles;i++) {
      bounds.extend(particle[i].position);
      attr_lo = std::min(attr_lo,particle[i].attribute);
      attr_hi = std::max(attr_hi,particle[i].attribute);
    }
    PRINT(bounds);
    PRINT(attr_lo);
    PRINT(attr_hi);

    box3f centerBounds = bounds;
    box3f sphereBounds = bounds;
    sphereBounds.lower -= vec3f(radius);
    sphereBounds.upper += vec3f(radius);
    updateInnerNodeInfo(bounds,0);

    PING;
    ispc::PKDGeometry_set(getIE(),
                          model->getIE(),
                          transferFunction->getIE(),
                          radius,
                          (ispc::PKDParticle*)particle, numParticles,
                          (uint32*)innerNodeInfo, numInnerNodes,
                          (ispc::box3f &)centerBounds,
                          (ispc::box3f &)sphereBounds,
                          attr_lo,attr_hi);
    PING;
  }
  
  uint32 PKDGeometry::makeRangeBits(float attribute)
  {
    float rel_attribute = (attribute - attr_lo) / (attr_hi - attr_lo);
    int binID = std::min(31,int(rel_attribute*30));
    return 1<<binID;
  }

  uint32 PKDGeometry::updateInnerNodeInfo(const box3f &bounds, size_t nodeID)
  {
    if (nodeID >= numParticles) 
      return 0;

    if (!embree::conjoint(bounds,particle[nodeID].position)) {
      PING;
      PRINT(bounds);
      PRINT(nodeID);
      PRINT(particle[nodeID].position);
    }

    if (nodeID >= numInnerNodes) 
      return makeRangeBits(particle[nodeID].attribute);
    else {
      size_t splitDim = maxDim(bounds.size());
      innerNodeInfo[nodeID].setDim(splitDim);
      uint32 bits = 0;
      box3f lBounds = bounds, rBounds = bounds;
      lBounds.upper[splitDim] = rBounds.lower[splitDim] = particle[nodeID].position[splitDim];
      bits |= updateInnerNodeInfo(lBounds,2*nodeID+1);
      bits |= updateInnerNodeInfo(rBounds,2*nodeID+2);
      innerNodeInfo[nodeID].setBinBits(bits);
      return innerNodeInfo[nodeID].getBinBits();
    }
  }



  OSP_REGISTER_GEOMETRY(AlphaSpheres,alpha_spheres);
  OSP_REGISTER_GEOMETRY(PKDGeometry,pkd_geometry);

  extern "C" void ospray_init_module_alpha_spheres() 
  {
    printf("Loaded plugin 'alpha_spheres' ...\n");
  }

}
