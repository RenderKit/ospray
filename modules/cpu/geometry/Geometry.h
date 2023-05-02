// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "common/Data.h"
#include "common/FeatureFlagsEnum.h"
#include "common/ObjectFactory.h"
// embree
#include "common/Embree.h"
// ispc shared
#include "GeometryShared.h"

namespace ospray {

struct OSPRAY_SDK_INTERFACE Geometry
    : public AddStructShared<ISPCDeviceObject, ispc::Geometry>,
      public ObjectFactory<Geometry, api::ISPCDevice &>
{
  Geometry(api::ISPCDevice &device, const FeatureFlagsGeometry ffg);
  virtual ~Geometry() override;

  virtual std::string toString() const override;

  virtual size_t numPrimitives() const = 0;

  void postCreationInfo(size_t numVerts = 0) const;

  RTCGeometry getEmbreeGeometry() const;

  bool supportAreaLighting() const;

  virtual FeatureFlags getFeatureFlags() const;

 protected:
  RTCGeometry embreeGeometry{nullptr};

  FeatureFlagsGeometry featureFlagsGeometry;

  void createEmbreeGeometry(RTCGeometryType type);
  // NOTE: We now pass intersection functions through Embree RTCIntersectionArgs
  // context parameter so that they can be inlined in SYCL
  void createEmbreeUserGeometry(RTCBoundsFunction boundsFn);
};

OSPTYPEFOR_SPECIALIZATION(Geometry *, OSP_GEOMETRY);

inline RTCGeometry Geometry::getEmbreeGeometry() const
{
  return embreeGeometry;
}

inline bool Geometry::supportAreaLighting() const
{
  return (getSh()->sampleArea != nullptr) && (getSh()->getAreas != nullptr);
}

inline FeatureFlags Geometry::getFeatureFlags() const
{
  FeatureFlags ff;
  ff.geometry = featureFlagsGeometry;
  return ff;
}

// convenience wrappers to set Embree buffer //////////////////////////////////

template <typename T>
struct RTCFormatFor
{
  static constexpr RTCFormat value = RTC_FORMAT_UNDEFINED;
};

#define RTCFORMATFOR_SPECIALIZATION(type, rtc_format)                          \
  template <>                                                                  \
  struct RTCFormatFor<type>                                                    \
  {                                                                            \
    static constexpr RTCFormat value = rtc_format;                             \
  };

RTCFORMATFOR_SPECIALIZATION(char, RTC_FORMAT_CHAR);
RTCFORMATFOR_SPECIALIZATION(unsigned char, RTC_FORMAT_UCHAR);
RTCFORMATFOR_SPECIALIZATION(short, RTC_FORMAT_SHORT);
RTCFORMATFOR_SPECIALIZATION(unsigned short, RTC_FORMAT_USHORT);
RTCFORMATFOR_SPECIALIZATION(int32_t, RTC_FORMAT_INT);
RTCFORMATFOR_SPECIALIZATION(vec2i, RTC_FORMAT_INT2);
RTCFORMATFOR_SPECIALIZATION(vec3i, RTC_FORMAT_INT3);
RTCFORMATFOR_SPECIALIZATION(vec4i, RTC_FORMAT_INT4);
RTCFORMATFOR_SPECIALIZATION(uint32_t, RTC_FORMAT_UINT);
RTCFORMATFOR_SPECIALIZATION(vec2ui, RTC_FORMAT_UINT2);
RTCFORMATFOR_SPECIALIZATION(vec3ui, RTC_FORMAT_UINT3);
RTCFORMATFOR_SPECIALIZATION(vec4ui, RTC_FORMAT_UINT4);
RTCFORMATFOR_SPECIALIZATION(int64_t, RTC_FORMAT_LLONG);
RTCFORMATFOR_SPECIALIZATION(vec2l, RTC_FORMAT_LLONG2);
RTCFORMATFOR_SPECIALIZATION(vec3l, RTC_FORMAT_LLONG3);
RTCFORMATFOR_SPECIALIZATION(vec4l, RTC_FORMAT_LLONG4);
RTCFORMATFOR_SPECIALIZATION(uint64_t, RTC_FORMAT_ULLONG);
RTCFORMATFOR_SPECIALIZATION(vec2ul, RTC_FORMAT_ULLONG2);
RTCFORMATFOR_SPECIALIZATION(vec3ul, RTC_FORMAT_ULLONG3);
RTCFORMATFOR_SPECIALIZATION(vec4ul, RTC_FORMAT_ULLONG4);
RTCFORMATFOR_SPECIALIZATION(float, RTC_FORMAT_FLOAT);
RTCFORMATFOR_SPECIALIZATION(vec2f, RTC_FORMAT_FLOAT2);
RTCFORMATFOR_SPECIALIZATION(vec3f, RTC_FORMAT_FLOAT3);
RTCFORMATFOR_SPECIALIZATION(vec4f, RTC_FORMAT_FLOAT4);
RTCFORMATFOR_SPECIALIZATION(linear2f, RTC_FORMAT_FLOAT2X2_COLUMN_MAJOR);
RTCFORMATFOR_SPECIALIZATION(linear3f, RTC_FORMAT_FLOAT3X3_COLUMN_MAJOR);
RTCFORMATFOR_SPECIALIZATION(affine2f, RTC_FORMAT_FLOAT2X3_COLUMN_MAJOR);
RTCFORMATFOR_SPECIALIZATION(affine3f, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR);
#undef RTCFORMATFOR_SPECIALIZATION

// ... via Ref<ospData>, NoOp when data is invalid
template <typename T>
void setEmbreeGeometryBuffer(RTCGeometry geom,
    enum RTCBufferType type,
    Ref<const DataT<T, 1>> &dataRef,
    unsigned int slot = 0)
{
  if (!dataRef)
    return;
  rtcSetSharedGeometryBuffer(geom,
      type,
      slot,
      RTCFormatFor<T>::value,
      dataRef->data(),
      0,
      dataRef->stride(),
      dataRef->size());
}
// ... via *ospData, NoOp when data is invalid
template <typename T>
void setEmbreeGeometryBuffer(RTCGeometry geom,
    enum RTCBufferType type,
    const DataT<T, 1> *dataPtr,
    unsigned int slot = 0)
{
  if (!dataPtr)
    return;
  rtcSetSharedGeometryBuffer(geom,
      type,
      slot,
      RTCFormatFor<T>::value,
      dataPtr->data(),
      0,
      dataPtr->stride(),
      dataPtr->size());
}
// ... via an std::vector
template <typename T>
void setEmbreeGeometryBuffer(RTCGeometry geom,
    enum RTCBufferType type,
    std::vector<T> &data,
    unsigned int slot = 0)
{
  rtcSetSharedGeometryBuffer(geom,
      type,
      slot,
      RTCFormatFor<T>::value,
      data.data(),
      0,
      sizeof(T),
      data.size());
}

} // namespace ospray
