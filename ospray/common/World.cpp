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
#include "World.h"
#include "Instance.h"
#include "api/ISPCDevice.h"
// ispc exports
#include "World_ispc.h"

namespace ospray {

  extern "C" void *ospray_getEmbreeDevice()
  {
    return api::ISPCDevice::embreeDevice;
  }

  // Embree helper functions ///////////////////////////////////////////////////

  static std::pair<int, int> createEmbreeScenes(RTCScene &geometryScene,
                                                RTCScene &volumeScene,
                                                Data &instances,
                                                int embreeFlags)
  {
    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    geometryScene = rtcNewScene(embreeDevice);
    volumeScene   = rtcNewScene(embreeDevice);

    int numGeomInstances   = 0;
    int numVolumeInstances = 0;

    auto begin = instances.begin<Instance *>();
    auto end   = instances.end<Instance *>();
    std::for_each(begin, end, [&](Instance *inst) {
      auto instGeometryScene = inst->embreeGeometryScene();
      auto instVolumeScene   = inst->embreeVolumeScene();

      auto xfm = inst->xfm();

      if (instGeometryScene) {
        auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
        rtcSetGeometryInstancedScene(eInst, instGeometryScene.value());
        rtcSetGeometryTransform(
            eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
        rtcCommitGeometry(eInst);

        rtcAttachGeometry(geometryScene, eInst);

        rtcReleaseGeometry(eInst);

        numGeomInstances++;
      }

      if (instVolumeScene) {
        auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
        rtcSetGeometryInstancedScene(eInst, instVolumeScene.value());
        rtcSetGeometryTransform(
            eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
        rtcCommitGeometry(eInst);

        rtcAttachGeometry(volumeScene, eInst);

        rtcReleaseGeometry(eInst);

        numVolumeInstances++;
      }
    });

    rtcSetSceneFlags(geometryScene, static_cast<RTCSceneFlags>(embreeFlags));
    rtcSetSceneFlags(volumeScene, static_cast<RTCSceneFlags>(embreeFlags));

    rtcCommitScene(geometryScene);
    rtcCommitScene(volumeScene);

    return std::make_pair(numGeomInstances, numVolumeInstances);
  }

  static void freeAndNullifyEmbreeScene(RTCScene &scene)
  {
    if (scene)
      rtcReleaseScene(scene);

    scene = nullptr;
  }

  // World definitions ////////////////////////////////////////////////////////

  World::World()
  {
    managedObjectType    = OSP_WORLD;
    this->ispcEquivalent = ispc::World_create(this);
  }

  World::~World()
  {
    freeAndNullifyEmbreeScene(embreeSceneHandleGeometries);
    freeAndNullifyEmbreeScene(embreeSceneHandleVolumes);
  }

  std::string World::toString() const
  {
    return "ospray::World";
  }

  void World::commit()
  {
    numGeometries = 0;
    numVolumes    = 0;

    freeAndNullifyEmbreeScene(embreeSceneHandleGeometries);
    freeAndNullifyEmbreeScene(embreeSceneHandleVolumes);

    instances = (Data *)getParamObject("instances");

    if (!instances)
      return;

    auto numInstances = instances->size();

    int sceneFlags = 0;
    sceneFlags |=
        (getParam<bool>("dynamicScene", false) ? RTC_SCENE_FLAG_DYNAMIC : 0);
    sceneFlags |=
        (getParam<bool>("compactMode", false) ? RTC_SCENE_FLAG_COMPACT : 0);
    sceneFlags |=
        (getParam<bool>("robustMode", false) ? RTC_SCENE_FLAG_ROBUST : 0);

    postStatusMsg(2)
        << "=======================================================\n"
        << "Committing world, which has " << numInstances << " instances";

    auto numGeomsAndVolumes = createEmbreeScenes(embreeSceneHandleGeometries,
                                                 embreeSceneHandleVolumes,
                                                 *instances,
                                                 sceneFlags);

    numGeometries = numGeomsAndVolumes.first;
    numVolumes    = numGeomsAndVolumes.second;

    instanceIEs = createArrayOfIE(*instances);

    ispc::World_set(getIE(),
                    instanceIEs.data(),
                    numInstances,
                    embreeSceneHandleGeometries,
                    embreeSceneHandleVolumes);
  }

}  // namespace ospray
