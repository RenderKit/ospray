// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ospray
#include "Model.h"
#include "geometry/TriangleMesh.h"
// embree
#include "embree2/rtcore.h"
#include "embree2/rtcore_scene.h"
#include "embree2/rtcore_geometry.h"
// ispc exports
#include "Model_ispc.h"


namespace ospray {

  using std::cout;
  using std::endl;

  extern RTCDevice g_embreeDevice;

  extern "C" void *ospray_getEmbreeDevice() { return g_embreeDevice; }

  Model::Model()
  {
    managedObjectType = OSP_MODEL;
    this->ispcEquivalent = ispc::Model_create(this);
    this->embreeSceneHandle = NULL;
  }
  void Model::finalize()
  {
    if (logLevel >= 2) {
      std::cout << "=======================================================" << std::endl;
      std::cout << "Finalizing model, has " 
           << geometry.size() << " geometries and " << volume.size() << " volumes" << std::endl << std::flush;
    }

    ispc::Model_init(getIE(), g_embreeDevice, geometry.size(), volume.size());
    embreeSceneHandle = (RTCScene)ispc::Model_getEmbreeSceneHandle(getIE());

    bounds = empty;

    // for now, only implement triangular geometry...
    for (size_t i=0; i < geometry.size(); i++) {

      if (logLevel >= 2) {
        std::cout << "=======================================================" << std::endl;
        std::cout << "Finalizing geometry " << i << std::endl << std::flush;
      }

      geometry[i]->finalize(this);

      bounds.extend(geometry[i]->bounds);
      ispc::Model_setGeometry(getIE(), i, geometry[i]->getIE());
    }

    for (size_t i=0; i<volume.size(); i++) 
      ispc::Model_setVolume(getIE(), i, volume[i]->getIE());
    
    rtcCommit(embreeSceneHandle);
  }

} // ::ospray
