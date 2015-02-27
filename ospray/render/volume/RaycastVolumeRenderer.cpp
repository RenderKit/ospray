// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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
#include "ospray/camera/PerspectiveCamera.h"
#include "ospray/common/Data.h"
#include "ospray/common/Ray.h"
#include "ospray/render/volume/RaycastVolumeRenderer.h"
#include "ospray/volume/Volume.h"
// ispc exports
#include "RaycastVolumeRenderer_ispc.h"

namespace ospray {

  void RaycastVolumeRenderer::commit() {

    //! Create the equivalent ISPC RaycastVolumeRenderer object.
    if (ispcEquivalent == NULL) ispcEquivalent = ispc::RaycastVolumeRenderer_createInstance();

    //! Get the camera.
    camera = (Camera *) getParamObject("camera", NULL);  exitOnCondition(camera == NULL, "no camera specified");

    //! Get the model.
    model = (Model *) getParamObject("model", NULL);  exitOnCondition(model == NULL, "no model specified");

    //! Get the dynamic model.
    dynamicModel = (Model *) getParamObject("dynamic_model", NULL);  exitOnCondition(dynamicModel == NULL, "no dynamic model specified");

    //! Set the camera.
    ispc::RaycastVolumeRenderer_setCamera(ispcEquivalent, camera->getIE());

    //! Set the model.
    ispc::RaycastVolumeRenderer_setModel(ispcEquivalent, model->getIE());

    //! Set the dynamic model.
    ispc::RaycastVolumeRenderer_setDynamicModel(ispcEquivalent, dynamicModel->getIE());

    //! Set the lights if any.
    ispc::RaycastVolumeRenderer_setLights(ispcEquivalent, getLightsFromData(getParamData("lights", NULL)));

    //! Initialize state in the parent class, must be called after the ISPC object is created.
    Renderer::commit();

  }

  void **RaycastVolumeRenderer::getLightsFromData(const Data *buffer) {

    //! Lights are optional.
    size_t lightCount = (buffer != NULL) ? buffer->numItems : 0;

    //! The light array is a NULL terminated list of pointers.
    void **lights = new void *[lightCount + 1];

    //! Copy pointers to the ISPC Light objects.
    for (size_t i=0 ; i < lightCount ; i++) lights[i] = ((Light **) buffer->data)[i]->getIE();

    //! Mark the end of the array.
    lights[lightCount] = NULL;  return(lights);

  }

} // ::ospray

