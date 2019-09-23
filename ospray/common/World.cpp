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
      const DataT<Instance *> &instances,
      int embreeFlags)
  {
    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    int numGeomInstances   = 0;
    int numVolumeInstances = 0;

    for (auto &&inst : instances) {
      auto instGeometryScene = inst->group->embreeGeometryScene();
      auto instVolumeScene   = inst->group->embreeVolumeScene();

      auto xfm = inst->xfm();

      {
        auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
        rtcSetGeometryInstancedScene(eInst, instGeometryScene.value());
        rtcSetGeometryTransform(
            eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
        rtcCommitGeometry(eInst);

        rtcAttachGeometry(geometryScene, eInst);

        numGeomInstances++;
      }

      {
        auto eInst = rtcNewGeometry(embreeDevice, RTC_GEOMETRY_TYPE_INSTANCE);
        rtcSetGeometryInstancedScene(eInst, instVolumeScene.value());
        rtcSetGeometryTransform(
            eInst, 0, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);
        rtcCommitGeometry(eInst);

        rtcAttachGeometry(volumeScene, eInst);

        numVolumeInstances++;
      }
    }

    rtcSetSceneFlags(geometryScene, static_cast<RTCSceneFlags>(embreeFlags));
    rtcSetSceneFlags(volumeScene, static_cast<RTCSceneFlags>(embreeFlags));

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

    instances = getParamDataT<Instance *>("instance");

    // get rid of stride for now
    if (instances && !instances->compact()) {
      auto data =
          new Data(OSP_GEOMETRIC_MODEL, vec3ui(instances->size(), 1, 1));
      data->copy(*instances, vec3ui(0));
      instances = &(data->as<Instance *>());
      data->refDec();
    }

    auto numInstances = instances ? instances->size() : 0;

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

    RTCDevice embreeDevice = (RTCDevice)ospray_getEmbreeDevice();

    embreeSceneHandleGeometries = rtcNewScene(embreeDevice);
    embreeSceneHandleVolumes    = rtcNewScene(embreeDevice);

    if (instances) {
      auto numGeomsAndVolumes = createEmbreeScenes(embreeSceneHandleGeometries,
                                                   embreeSceneHandleVolumes,
                                                   *instances,
                                                   sceneFlags);

      numGeometries = numGeomsAndVolumes.first;
      numVolumes    = numGeomsAndVolumes.second;

      instanceIEs = createArrayOfIE(*instances);
    }

    rtcCommitScene(embreeSceneHandleGeometries);
    rtcCommitScene(embreeSceneHandleVolumes);

    ispc::World_set(getIE(),
                    instanceIEs.data(),
                    numInstances,
                    embreeSceneHandleGeometries,
                    embreeSceneHandleVolumes);
  }

}  // namespace ospray
