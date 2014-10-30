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

  AlphaSpheres::AlphaSpheres()
  {
    this->ispcEquivalent = ispc::AlphaSpheres_create(this);
    // _materialList = NULL;
  }
  
  void AlphaSpheres::buildBVH() 
  {
    PrimAbstraction pa(sphere,numSpheres);
    mmBVH.build(&pa);
  }

  void AlphaSpheres::finalize(Model *model) 
  {
    // radius            = getParam1f("radius",0.01f);
    // materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",6*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1); //3*sizeof(float));
    offset_attribute  = getParam1i("offset_attribute",-1);
    sphereData          = getParamData("spheres",NULL);
    // materialList      = getParamData("materialList",NULL);
    transferFunction  = (TransferFunction *)getParamObject("transferFunction",NULL);
    
    if (sphereData == NULL) 
      throw std::runtime_error("#osp:AlphaSpheres: no 'spheres' data specified");
    numSpheres = sphereData->numBytes / bytesPerSphere;

    std::cout << "#osp: creating 'alpha_spheres' geometry, #spheres = " << numSpheres << std::endl;
    
    if (numSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::Spheres: too many spheres in this sphere geometry. Consider splitting this geometry in multiple geometries with fewer spheres (you can still put all those geometries into a single model, but you can't put that many spheres into a single geometry without causing address overflows)");
    }
    assert(transferFunction);
    
    sphere = (Sphere *)sphereData->data;
    PRINT(sphere);

    buildBVH();
    PING;

    ispc::AlphaSpheres_set(getIE(),
                           model->getIE(),
                           transferFunction->getIE(),
                           mmBVH.rootRef,
                           mmBVH.getNodePtr(),
                           sphere,
                           numSpheres,bytesPerSphere,
                           offset_center,offset_radius,
                           offset_attribute);
    PRINT(mmBVH.node.size());
    PRINT(mmBVH.node[0]);
    PING;
  }


  OSP_REGISTER_GEOMETRY(AlphaSpheres,alpha_spheres);

  extern "C" void ospray_init_module_alpha_spheres() 
  {
    printf("Loaded plugin 'alpha_spheres' ...\n");
  }

}
