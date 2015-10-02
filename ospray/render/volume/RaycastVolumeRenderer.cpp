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

// #if EXP_DATA_PARALLEL
//     //! \brief render-frame call back for data parallel mode
//     /*! \detailed in data parallel mode, we have to render multiple
//         'passes' (depending on many pieces there are), so we have to
//         override the default renderframe function */
//   void RaycastVolumeRenderer::renderFrame(FrameBuffer *fb, const uint32 channelFlags)
//   {
//     bool isDataParallel = false;
//     uint32 numMyPieces = 0;
//     for (int i=0;model->volume.size();i++) {
//       size_t totalPieces, myPieces;
//       isDataParallel |= model->volume[i]->getDataParallelInfo(totalPieces,myPieces);
//     }
//     DistributedFrameBuffer *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
//     if (!dfb)
//       throw std::runtime_error("OSPRay data parallel rendering error. "
//                                "this is a data-parallel scene, but we're "
//                                "not using the distributed frame buffer!?");
//   }
// #endif

  void RaycastVolumeRenderer::commit() 
  {
    // Create the equivalent ISPC RaycastVolumeRenderer object.
    if (ispcEquivalent == NULL) ispcEquivalent = ispc::RaycastVolumeRenderer_createInstance();

    // Get the background color.
    vec3f bgColor = getParam3f("bgColor", vec3f(1.f));

    // Set the background color.
    ispc::RaycastVolumeRenderer_setBackgroundColor(ispcEquivalent, (const ispc::vec3f&) bgColor);

    // Set the lights if any.
    Data *lightsData = (Data *)getParamData("lights", NULL);

    lights.clear();

    if (lightsData)
      for (size_t i=0; i<lightsData->size(); i++)
        lights.push_back(((Light **)lightsData->data)[i]->getIE());

    ispc::RaycastVolumeRenderer_setLights(ispcEquivalent, 
                                          lights.empty() ? NULL : &lights[0], 
                                          lights.size());

    // Initialize state in the parent class, must be called after the ISPC object is created.
    Renderer::commit();
  }

  // A renderer type for volumes with embedded surfaces.
  OSP_REGISTER_RENDERER(RaycastVolumeRenderer, raycast_volume_renderer);

} // ::ospray

