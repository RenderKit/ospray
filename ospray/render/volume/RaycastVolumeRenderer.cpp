//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/camera/perspectivecamera.h"
#include "ospray/common/Data.h"
#include "ospray/common/Ray.h"
#include "ospray/render/volume/RaycastVolumeRenderer.h"
#include "ospray/volume/Volume.h"
#include "RaycastVolumeRenderer_ispc.h"

namespace ospray {

    void RaycastVolumeRenderer::commit() {

        //! Create the equivalent ISPC RaycastVolumeRenderer object.
        if (ispcEquivalent == NULL) ispcEquivalent = ispc::RaycastVolumeRenderer_createInstance();

        //! Get the camera.
        camera = (Camera *) getParamObject("camera", NULL);  exitOnCondition(camera == NULL, "no camera specified");

        //! Get the model.
        model = (Model *) getParamObject("model", NULL);  exitOnCondition(model == NULL, "no model specified");

        //! Set the camera.
        ispc::RaycastVolumeRenderer_setCamera(ispcEquivalent, camera->getIE());

        //! Set the model.
        ispc::RaycastVolumeRenderer_setModel(ispcEquivalent, model->getIE());

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

} // namespace ospray

