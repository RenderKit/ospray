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
#include "GeometryInstance.h"
// ispc exports
#include "GeometryInstance_ispc.h"

namespace ospray {

  GeometryInstance::GeometryInstance(Geometry *geometry)
  {
    if (geometry == nullptr)
      throw std::runtime_error("NULL Geometry given to GeometryInstance!");

    instancedGeometry = geometry;

    this->ispcEquivalent =
        ispc::GeometryInstance_create(this, geometry->getIE());

    setMaterialList(nullptr);
  }

  GeometryInstance::~GeometryInstance()
  {
    if (embreeInstanceGeometry)
      rtcReleaseGeometry(embreeInstanceGeometry);

    if (embreeSceneHandle)
      rtcReleaseScene(embreeSceneHandle);
  }

  std::string GeometryInstance::toString() const
  {
    return "ospray::GeometryInstance";
  }

  void GeometryInstance::setMaterial(Material *mat)
  {
    OSPMaterial ospMat = (OSPMaterial)mat;
    auto *data         = new Data(1, OSP_OBJECT, &ospMat);
    setMaterialList(data);
    data->refDec();
  }

  void GeometryInstance::setMaterialList(Data *matListData)
  {
    if (!matListData || matListData->numItems == 0) {
      ispc::GeometryInstance_setMaterialList(this->getIE(), 0, nullptr);
      return;
    }

    materialListData = matListData;
    materialList     = (Material **)materialListData->data;

    if (!getIE()) {
      postStatusMsg(
          "#osp: warning: geometry does not have an "
          "ispc equivalent!");
    } else {
      const int numMaterials = materialListData->numItems;
      ispcMaterialPtrs.resize(numMaterials);
      for (int i = 0; i < numMaterials; i++)
        ispcMaterialPtrs[i] = materialList[i]->getIE();

      ispc::GeometryInstance_setMaterialList(
          this->getIE(), numMaterials, ispcMaterialPtrs.data());
    }
  }

  void GeometryInstance::commit()
  {
    // Get transform information //

    instanceXfm.l.vx = getParam3f("xfm.l.vx", vec3f(1.f, 0.f, 0.f));
    instanceXfm.l.vy = getParam3f("xfm.l.vy", vec3f(0.f, 1.f, 0.f));
    instanceXfm.l.vz = getParam3f("xfm.l.vz", vec3f(0.f, 0.f, 1.f));
    instanceXfm.p    = getParam3f("xfm.p", vec3f(0.f, 0.f, 0.f));

    // Create Embree instanced scene, if necessary //

    if (lastEmbreeInstanceGeometryHandle != instancedGeometry->embreeGeometry) {
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

      lastEmbreeInstanceGeometryHandle = instancedGeometry->embreeGeometry;

      embreeID = rtcAttachGeometry(embreeSceneHandle,
                                   instancedGeometry->embreeGeometry);
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

    const box3f b = instancedGeometry->bounds;
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

    AffineSpace3f rcp_xfm = rcp(instanceXfm);

    // Finish getting/setting other appearance information //

    colorData = getParamData("color", getParamData("prim.color"));

    if (colorData &&
        colorData->numItems != instancedGeometry->numPrimitives()) {
      throw std::runtime_error(
          "number of colors does not match number of primitives!");
    }

    prim_materialIDData       = getParamData("prim.materialID");
    Data *materialListDataPtr = getParamData("materialList");
    if (materialListDataPtr)
      setMaterialList(materialListDataPtr);

    ispc::GeometryInstance_set(
        getIE(),
        (ispc::AffineSpace3f &)instanceXfm,
        (ispc::AffineSpace3f &)rcp_xfm,
        colorData ? colorData->data : nullptr,
        prim_materialIDData ? prim_materialIDData->data : nullptr);
  }

  RTCGeometry GeometryInstance::embreeGeometryHandle() const
  {
    return embreeInstanceGeometry;
  }

  box3f GeometryInstance::bounds() const
  {
    return instanceBounds;
  }

  AffineSpace3f GeometryInstance::xfm() const
  {
    return instanceXfm;
  }

}  // namespace ospray
