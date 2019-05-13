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
#include "VolumeInstance.h"
#include "transferFunction/TransferFunction.h"
// ispc exports
#include "VolumeInstance_ispc.h"

namespace ospray {

  VolumeInstance::VolumeInstance(Volume *volume)
  {
    if (volume == nullptr)
      throw std::runtime_error("NULL Volume given to VolumeInstance!");

    instancedVolume = volume;

    this->ispcEquivalent = ispc::VolumeInstance_create(this, volume->getIE());
  }

  VolumeInstance::~VolumeInstance()
  {
    if (embreeInstanceGeometry)
      rtcReleaseGeometry(embreeInstanceGeometry);

    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);
  }

  std::string VolumeInstance::toString() const
  {
    return "ospray::VolumeInstance";
  }

  void VolumeInstance::commit()
  {
    auto *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error("no transfer function specified on the volume!");

    // Get transform information //

    instanceXfm.l.vx = getParam3f("xfm.l.vx", vec3f(1.f, 0.f, 0.f));
    instanceXfm.l.vy = getParam3f("xfm.l.vy", vec3f(0.f, 1.f, 0.f));
    instanceXfm.l.vz = getParam3f("xfm.l.vz", vec3f(0.f, 0.f, 1.f));
    instanceXfm.p    = getParam3f("xfm.p", vec3f(0.f, 0.f, 0.f));

    // Create Embree instanced scene, if necessary //

    if (lastEmbreeInstanceGeometryHandle != instancedVolume->embreeGeometry) {
      if (embreeSceneHandle) {
        rtcDetachGeometry(embreeSceneHandle, embreeID);
        rtcReleaseScene(embreeSceneHandle);
      }

      embreeSceneHandle = rtcNewScene(ispc_embreeDevice());

      if (embreeInstanceGeometry)
        rtcReleaseGeometry(embreeInstanceGeometry);

      embreeInstanceGeometry =
          rtcNewGeometry(ispc_embreeDevice(), RTC_GEOMETRY_TYPE_INSTANCE);

      bool useEmbreeDynamicSceneFlag = getParam1b("dynamicScene", 0);
      bool useEmbreeCompactSceneFlag = getParam1b("compactMode", 0);
      bool useEmbreeRobustSceneFlag  = getParam1b("robustMode", 0);

      int sceneFlags = 0;
      sceneFlags |= (useEmbreeDynamicSceneFlag ? RTC_SCENE_FLAG_DYNAMIC : 0);
      sceneFlags |= (useEmbreeCompactSceneFlag ? RTC_SCENE_FLAG_COMPACT : 0);
      sceneFlags |= (useEmbreeRobustSceneFlag ? RTC_SCENE_FLAG_ROBUST : 0);

      rtcSetSceneFlags(embreeSceneHandle,
                       static_cast<RTCSceneFlags>(sceneFlags));

      lastEmbreeInstanceGeometryHandle = instancedVolume->embreeGeometry;
      embreeID =
          rtcAttachGeometry(embreeSceneHandle, instancedVolume->embreeGeometry);
      rtcCommitScene(embreeSceneHandle);

      rtcSetGeometryInstancedScene(embreeInstanceGeometry, embreeSceneHandle);
    }

    // Set Embree transform //

    rtcSetGeometryTransform(embreeInstanceGeometry,
                            0,
                            RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
                            &instanceXfm);

    rtcCommitGeometry(embreeInstanceGeometry);

    // Calculate bounding information //

    const box3f b = instancedVolume->bounds;
    const vec3f v000(b.lower.x, b.lower.y, b.lower.z);
    const vec3f v001(b.upper.x, b.lower.y, b.lower.z);
    const vec3f v010(b.lower.x, b.upper.y, b.lower.z);
    const vec3f v011(b.upper.x, b.upper.y, b.lower.z);
    const vec3f v100(b.lower.x, b.lower.y, b.upper.z);
    const vec3f v101(b.upper.x, b.lower.y, b.upper.z);
    const vec3f v110(b.lower.x, b.upper.y, b.upper.z);
    const vec3f v111(b.upper.x, b.upper.y, b.upper.z);

    instanceBounds = empty;
    instanceBounds.extend(xfmPoint(instanceXfm, v000));
    instanceBounds.extend(xfmPoint(instanceXfm, v001));
    instanceBounds.extend(xfmPoint(instanceXfm, v010));
    instanceBounds.extend(xfmPoint(instanceXfm, v011));
    instanceBounds.extend(xfmPoint(instanceXfm, v100));
    instanceBounds.extend(xfmPoint(instanceXfm, v101));
    instanceBounds.extend(xfmPoint(instanceXfm, v110));
    instanceBounds.extend(xfmPoint(instanceXfm, v111));

    auto rcp_xfm = rcp(instanceXfm);

    // Finish getting/setting other appearance information //

    box3f volumeClippingBox =
        box3f(getParam3f("volumeClippingBoxLower", vec3f(0.f)),
              getParam3f("volumeClippingBoxUpper", vec3f(0.f)));

    ispc::VolumeInstance_set(ispcEquivalent,
                             getParam1b("preIntegration", false),
                             getParam1b("adaptiveSampling", true),
                             getParam1f("adaptiveScalar", 15.0f),
                             getParam1f("adaptiveMaxSamplingRate", 2.0f),
                             getParam1f("adaptiveBacktrack", 0.03f),
                             getParam1f("samplingRate", 0.125f),
                             transferFunction->getIE(),
                             (const ispc::box3f &)volumeClippingBox,
                             (const ispc::box3f &)instanceBounds,
                             (ispc::AffineSpace3f &)instanceXfm,
                             (ispc::AffineSpace3f &)rcp_xfm);
  }

  RTCGeometry VolumeInstance::embreeGeometryHandle() const
  {
    return embreeInstanceGeometry;
  }

  box3f VolumeInstance::bounds() const
  {
    return instanceBounds;
  }

  AffineSpace3f VolumeInstance::xfm() const
  {
    return instanceXfm;
  }

}  // namespace ospray
