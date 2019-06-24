// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
#include "Instance.h"
// ispc exports
#include "Geometry_ispc.h"
#include "Instance_ispc.h"

namespace ospray {

  extern "C" void *ospray_getEmbreeDevice();

  // Embree helper functions ///////////////////////////////////////////////////

  template <typename T>
  void createEmbreeScene(RTCScene &scene, Data &objects, int embreeFlags)
  {
    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    scene = rtcNewScene(embreeDevice);

    auto begin = objects.begin<T *>();
    auto end   = objects.end<T *>();
    std::for_each(begin, end, [&](T *obj) {
      auto geomID = rtcAttachGeometry(scene, obj->embreeGeometryHandle());
      obj->setGeomID(geomID);
    });

    rtcSetSceneFlags(scene, static_cast<RTCSceneFlags>(embreeFlags));
    rtcCommitScene(scene);
  }

  static void freeAndNullifyEmbreeScene(RTCScene &scene)
  {
    if (scene)
      rtcReleaseScene(scene);

    scene = nullptr;
  }

  // Instance definitions /////////////////////////////////////////////////////

  Instance::Instance()
  {
    managedObjectType    = OSP_INSTANCE;
    this->ispcEquivalent = ispc::Instance_create(this);
  }

  Instance::~Instance()
  {
    freeAndNullifyEmbreeScene(sceneGeometries);
    freeAndNullifyEmbreeScene(sceneVolumes);
  }

  std::string Instance::toString() const
  {
    return "ospray::Instance";
  }

  void Instance::commit()
  {
    instanceXfm = getParamAffine3f("xfm", affine3f(one));
    rcpXfm      = rcp(instanceXfm);

    geometricModels  = (Data *)getParamObject("geometries");
    volumetricModels = (Data *)getParamObject("volumes");

    size_t numGeometries = geometricModels ? geometricModels->size() : 0;
    size_t numVolumes    = volumetricModels ? volumetricModels->size() : 0;

    postStatusMsg(2)
        << "=======================================================\n"
        << "Finalizing instance, which has " << numGeometries
        << " geometries and " << numVolumes << " volumes";

    int sceneFlags = 0;
    sceneFlags |=
        (getParam<bool>("dynamicScene", false) ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags |=
        (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags |=
        (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

    freeAndNullifyEmbreeScene(sceneGeometries);
    freeAndNullifyEmbreeScene(sceneVolumes);

    geometricModelIEs.clear();
    volumetricModelIEs.clear();

    if (numGeometries > 0) {
      createEmbreeScene<GeometricModel>(
          sceneGeometries, *geometricModels, sceneFlags);
      geometricModelIEs = createArrayOfIE(*geometricModels);
    }

    if (numVolumes > 0) {
      createEmbreeScene<VolumetricModel>(
          sceneVolumes, *volumetricModels, sceneFlags);
      volumetricModelIEs = createArrayOfIE(*volumetricModels);
    }

    ispc::Instance_set(getIE(),
                       geometricModels ? geometricModelIEs.data() : nullptr,
                       numGeometries,
                       volumetricModels ? volumetricModelIEs.data() : nullptr,
                       numVolumes,
                       (ispc::AffineSpace3f &)instanceXfm,
                       (ispc::AffineSpace3f &)rcpXfm);
  }

}  // namespace ospray
