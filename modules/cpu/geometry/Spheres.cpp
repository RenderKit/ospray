// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Spheres.h"
#include "common/Data.h"
#ifndef OSPRAY_TARGET_SYCL
// ispc-generated files
#include "geometry/Spheres_ispc.h"
#else
#include "geometry/Spheres.ih"
#endif

namespace ospray {

Spheres::Spheres(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtDevice(), device)
{
#ifndef OSPRAY_TARGET_SYCL
  getSh()->super.postIntersect =
      reinterpret_cast<ispc::Geometry_postIntersectFct>(
          ispc::Spheres_postIntersect_addr());
  getSh()->super.getAreas = reinterpret_cast<ispc::Geometry_GetAreasFct>(
      ispc::Spheres_getAreas_addr());
  getSh()->super.sampleArea = reinterpret_cast<ispc::Geometry_SampleAreaFct>(
      ispc::Spheres_sampleArea_addr());
#else
  getSh()->super.getAreas =
      reinterpret_cast<ispc::Geometry_GetAreasFct>(ispc::Spheres_getAreas);
  // We also set the sampleArea function ptr so that
  // Geometry::supportAreaLighting will be true, but in SYCL we'll never call it
  // through the function pointer on the device
  getSh()->super.sampleArea =
      reinterpret_cast<ispc::Geometry_SampleAreaFct>(ispc::Spheres_sampleArea);
#endif
}

std::string Spheres::toString() const
{
  return "ospray::Spheres";
}

void Spheres::commit()
{
  radius = getParam<float>("radius", 0.01f);
  vertexData = getParamDataT<vec3f>("sphere.position", true);
  radiusData = getParamDataT<float>("sphere.radius");
  texcoordData = getParamDataT<vec2f>("sphere.texcoord");

  // TODO: Param name for the new interleaved array?
  // sphere or sphere.data is a bit too generic because it kind of
  // implies the texcoord data is part of it? or isn't "data"?
  auto interleaved = new Data(getISPCDevice(), OSP_VEC4F, vertexData->numItems);
  sphereData = &interleaved->as<vec4f, 1>();
  interleaved->refDec();
  // For now default to always create the interleaved buffer since we
  // don't expose the interleaved data yet
  for (size_t i = 0; i < vertexData->size(); ++i) {
    float ptRadius = radius;
    if (radiusData) {
      ptRadius = (*radiusData)[i];
    }
    (*sphereData)[i] = vec4f((*vertexData)[i], ptRadius);
  }

#if 0
  createEmbreeGeometry(RTC_GEOMETRY_TYPE_SPHERE_POINT);
  rtcSetSharedGeometryBuffer(embreeGeometry,
      RTC_BUFFER_TYPE_VERTEX,
      0,
      RTC_FORMAT_FLOAT4,
      sphereData->data(),
      0,
      sizeof(vec4f),
      sphereData->size());
  rtcCommitGeometry(embreeGeometry);
#else
  // Testing user geometry issue on GPU
  createEmbreeUserGeometry((RTCBoundsFunction)&ispc::Spheres_bounds);
#endif

  getSh()->sphere = *ispc(sphereData);
  getSh()->texcoord = *ispc(texcoordData);
  getSh()->super.numPrimitives = numPrimitives();

  postCreationInfo(numPrimitives());
}

size_t Spheres::numPrimitives() const
{
  return sphereData ? sphereData->size() : 0;
}

} // namespace ospray
