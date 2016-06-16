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

#include "PathTracer.h"
// ospray
#include "ospray/common/Data.h"
#include "ospray/lights/Light.h"
#include "ospray/mpi/MPICommon.h"
// ispc exports
#include "PathTracer_ispc.h"
// std
#include <map>
#include <chrono>

#include "ospray/will_dbg_log.h"

namespace ospray {
  PathTracer::PathTracer() : Renderer()
  {
    ispcEquivalent = ispc::PathTracer_create(this);
  }

  /*! \brief create a material of given type */
  Material *PathTracer::createMaterial(const char *type)
  {
    std::string ptType = std::string("PathTracer_")+type;
    Material *material = Material::createMaterial(ptType.c_str());
    if (!material) {
      std::map<std::string,int> numOccurrances;
      const std::string T = type;
      if (numOccurrances[T] == 0)
        std::cout << "#osp:PT: does not know material type '" << type << "'" <<
          " (replacing with OBJMaterial)" << std::endl;
      numOccurrances[T] ++;
      material = Material::createMaterial("PathTracer_OBJMaterial");
      // throw std::runtime_error("invalid path tracer material "+std::string(type));
    }
    material->refInc();
    return material;
  }

  void PathTracer::commit()
  {
    Renderer::commit();

    lightData = (Data*)getParamData("lights");

    lightArray.clear();

    if (lightData)
      for (int i = 0; i < lightData->size(); i++)
        lightArray.push_back(((Light**)lightData->data)[i]->getIE());

    void **lightPtr = lightArray.empty() ? NULL : &lightArray[0];

    const int32 maxDepth = getParam1i("maxDepth", 20);
    const float minContribution = getParam1f("minContribution", 0.01f);
#if 1
    const float maxRadiance = getParam1f("maxContribution", getParam1f("maxRadiance", inf));
#else
    const float maxRadiance = getParam1f("maxContribution", getParam1f("maxRadiance", 10.f));
#endif
    Texture2D *backplate = (Texture2D*)getParamObject("backplate", NULL);

    ispc::PathTracer_set(getIE(), maxDepth, minContribution, maxRadiance,
                         backplate ? backplate->getIE() : NULL,
                         lightPtr, lightArray.size());
  }

  void* PathTracer::beginFrame(FrameBuffer *fb) {
    using namespace std::chrono;
    frameStart = high_resolution_clock::now();
    void *ret = Renderer::beginFrame(fb);
    return ret;
  }
  void PathTracer::endFrame(void *perFrameData, const int32 fbChannelFlags) {
    using namespace std::chrono;
    Renderer::endFrame(perFrameData, fbChannelFlags);

    frameEnd = high_resolution_clock::now();
    auto elapsed = duration_cast<milliseconds>(frameEnd - frameStart);
    // Skip the first 10 frames to warm up
    if (totalFrames > 10) {
      avgFrameTime = (elapsed + frameCount * avgFrameTime) / (frameCount + 1);
      ++frameCount;
    }
    ++totalFrames;

    if (frameCount > 0 && frameCount % 25 == 0) {
      std::cout << "Worker " << mpi::worker.rank << " avg frame time: "
        << avgFrameTime.count() << "ms over " << frameCount << " frames\n";
    }
  }

  float PathTracer::renderFrame(FrameBuffer *fb, const uint32 channelFlags) {
    const static int LOG_RANK = 0;
    using namespace std::chrono;

    WILL_DBG({
      std::cout << "Worker " << mpi::worker.rank << " start render frame # " << totalFrames << " at "
        << duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()
        << "ms\n";
    })
      
    float ret = Renderer::renderFrame(fb, channelFlags);

    WILL_DBG({
      std::cout << "Worker " << mpi::worker.rank << " end render frame at "
        << duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count()
        << "ms\n";
    })

    return ret;
  }

  OSP_REGISTER_RENDERER(PathTracer,pathtracer);
  OSP_REGISTER_RENDERER(PathTracer,pt);

  extern "C" void ospray_init_module_pathtracer()
  {
    printf("Loaded plugin 'pathtracer' ...\n");
  }
};

