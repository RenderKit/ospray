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

#undef NDEBUG

// ospray
#include "Spheres.h"
#include "common/Data.h"
#include "common/Model.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

  Spheres::Spheres()
  {
    this->ispcEquivalent = ispc::Spheres_create(this);
    _materialList = NULL;
  }

  Spheres::~Spheres()
  {
    if (_materialList) {
      free(_materialList);
      _materialList = NULL;
    }
  }

  void Spheres::finalize(Model *model) 
  {
    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1);
    offset_materialID = getParam1i("offset_materialID",-1);
    offset_colorID    = getParam1i("offset_colorID",-1);
    sphereData        = getParamData("spheres");
    materialList      = getParamData("materialList");
    colorData         = getParamData("color");
    colorOffset       = getParam1i("color_offset",0);
    colorStride       = getParam1i("color_stride",4*sizeof(float));

    if (sphereData.ptr == NULL) {
      throw std::runtime_error("#ospray:geometry/spheres: no 'spheres' data "
                               "specified");
    }

    numSpheres = sphereData->numBytes / bytesPerSphere;
    std::cout << "#osp: creating 'spheres' geometry, #spheres = " << numSpheres
              << std::endl;

    if (numSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::Spheres: too many spheres in this "
                               "sphere geometry. Consider splitting this "
                               "geometry in multiple geometries with fewer "
                               "spheres (you can still put all those "
                               "geometries into a single model, but you can't "
                               "put that many spheres into a single geometry "
                               "without causing address overflows)");
    }

    if (_materialList) {
      free(_materialList);
      _materialList = NULL;
    }

    if (materialList) {
      void **ispcMaterials = (void**) malloc(sizeof(void*) *
                                             materialList->numItems);
      for (int i=0;i<materialList->numItems;i++) {
        Material *m = ((Material**)materialList->data)[i];
        ispcMaterials[i] = m?m->getIE():NULL;
      }
      _materialList = (void*)ispcMaterials;
    }

    ispc::SpheresGeometry_set(getIE(),model->getIE(),
                              sphereData->data,_materialList,
                              colorData?(unsigned char*)colorData->data:NULL,
                              colorOffset, colorStride,
                              numSpheres,bytesPerSphere,
                              radius,materialID,
                              offset_center,offset_radius,
                              offset_materialID,offset_colorID);
  }

  OSP_REGISTER_GEOMETRY(Spheres,spheres);

} // ::ospray
