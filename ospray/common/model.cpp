/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "model.h"
// embree stuff
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
// ispc side
#include "model_ispc.h"

namespace ospray {
  using std::cout;
  using std::endl;

  Model::Model()
  {
    this->ispcEquivalent = ispc::Model_create(this);
  }
  void Model::finalize()
  {
    if (logLevel > 2) {
      cout << "=======================================================" << endl;
      cout << "Finalizing model, has " << geometry.size() << " geometries" << endl;
    }

    ispc::Model_init(getIE(), geometry.size());
    embreeSceneHandle = (RTCScene)ispc::Model_getEmbreeSceneHandle(getIE());
    
    // for now, only implement triangular geometry...
    for (size_t i=0; i < geometry.size(); i++) {
      geometry[i]->finalize(this);
      ispc::Model_setGeometry(getIE(), i, geometry[i]->getIE());
    }
    
    rtcCommit(embreeSceneHandle);
  }
}
