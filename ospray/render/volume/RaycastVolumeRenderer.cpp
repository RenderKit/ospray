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
#include "ospray/lights/Light.h"
#include "ospray/common/Data.h"
#include "ospray/render/volume/RaycastVolumeRenderer.h"
// ispc exports
#include "RaycastVolumeRenderer_ispc.h"
#include "RaycastVolumeRendererMaterial_ispc.h"

namespace ospray {

  RaycastVolumeRenderer::Material::Material()
  {
    ispcEquivalent = ispc::RaycastVolumeRendererMaterial_create(this);
  }

  void RaycastVolumeRenderer::Material::commit()
  {
    Kd = getParam3f("color", getParam3f("kd", getParam3f("Kd", vec3f(1.0f))));
    volume = (Volume *)getParamObject("volume", NULL);

    ispc::RaycastVolumeRendererMaterial_set(getIE(), (const ispc::vec3f&)Kd, volume ? volume->getIE() : NULL);
  }

  void RaycastVolumeRenderer::commit() 
  {
    // Create the equivalent ISPC RaycastVolumeRenderer object.
    if (ispcEquivalent == NULL) ispcEquivalent = ispc::RaycastVolumeRenderer_createInstance();

    // Get the background color.
    vec3f bgColor = getParam3f("bgColor", vec3f(1.f));

    // Set the background color.
    ispc::RaycastVolumeRenderer_setBackgroundColor(ispcEquivalent, (const ispc::vec3f&) bgColor);

    // Set the lights if any.
    ispc::RaycastVolumeRenderer_setLights(ispcEquivalent, getLightsFromData(getParamData("lights", NULL)));

    // Initialize state in the parent class, must be called after the ISPC object is created.
    Renderer::commit();
  }

  void **RaycastVolumeRenderer::getLightsFromData(const Data *buffer)
  {
    // Lights are optional.
    size_t lightCount = (buffer != NULL) ? buffer->numItems : 0;

    // The light array is a NULL terminated list of pointers.
    void **lights = new void *[lightCount + 1];

    // Copy pointers to the ISPC Light objects.
    for (size_t i=0 ; i < lightCount ; i++) lights[i] = ((Light **) buffer->data)[i]->getIE();

    // Mark the end of the array.
    lights[lightCount] = NULL;  return(lights);
  }

} // ::ospray

