/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

// ospray
#include "spheres.h"
#include "ospray/common/data.h"
#include "ospray/common/model.h"
// ispc-generated files
#include "spheres_ispc.h"

namespace ospray {

  Spheres::Spheres()
  {
    this->ispcEquivalent = ispc::Spheres_create(this);
    _materialList = NULL;
  }

  void Spheres::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1); //3*sizeof(float));
    offset_materialID = getParam1i("offset_materialID",-1);
    data              = getParamData("spheres",NULL);
    materialList      = getParamData("materialList",NULL);
    
    if (data == NULL) 
      throw std::runtime_error("#ospray:geometry/spheres: no 'spheres' data specified");
    numSpheres = data->numBytes / bytesPerSphere;
    std::cout << "#osp: creating 'spheres' geometry, #spheres = " << numSpheres << std::endl;
    
    if (_materialList) {
      free(_materialList);
      _materialList = NULL;
    }

    if (materialList) {
      void **ispcMaterials = (void**) malloc(sizeof(void*) * materialList->numItems);
      for (int i=0;i<materialList->numItems;i++) {
        Material *m = ((Material**)materialList->data)[i];
        ispcMaterials[i] = m?m->getIE():NULL;
      }
      _materialList = (void*)ispcMaterials;
    }
    ispc::SpheresGeometry_set(getIE(),model->getIE(),
                              data->data,_materialList,
                              numSpheres,bytesPerSphere,
                              radius,materialID,
                              offset_center,offset_radius,offset_materialID);
  }


  OSP_REGISTER_GEOMETRY(Spheres,spheres);
}
