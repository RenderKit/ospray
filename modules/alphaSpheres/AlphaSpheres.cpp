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

namespace ospray {
  using std::cout;
  using std::endl;

  AlphaSpheres::AlphaSpheres()
  {
    this->ispcEquivalent = ispc::AlphaSpheres_create(this);
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


  OSP_REGISTER_GEOMETRY(AlphaSpheres,alpha_spheres);

  extern "C" void ospray_init_module_alpha_spheres() 
  {
    printf("Loaded plugin 'alpha_spheres' ...\n");
  }

}
