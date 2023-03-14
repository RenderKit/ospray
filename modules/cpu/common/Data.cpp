// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray
#include "Data.h"
#include "common/ISPCRTBuffers.h"
#include "ospray/ospray.h"
#include "rkcommon/utility/multidim_index_sequence.h"

namespace ospray {

Data::Data(api::ISPCDevice &device,
    const void *sharedData,
    OSPDataType type,
    const vec3ul &numItems,
    const vec3l &byteStride)
    : ISPCDeviceObject(device),
      appSharedPtr((char *)sharedData),
      shared(true),
      type(type),
      numItems(numItems),
      byteStride(byteStride)
{
  if (sharedData == nullptr) {
    throw std::runtime_error("OSPData: shared buffer is NULL");
  }
  addr = appSharedPtr;
  init();

  if (isObjectType(type)) {
    for (auto &&child : as<ManagedObject *, 3>()) {
      if (child)
        child->refInc();
    }
  }
}

Data::Data(api::ISPCDevice &device, OSPDataType type, const vec3ul &numItems)
    : ISPCDeviceObject(device),
      shared(false),
      type(type),
      numItems(numItems),
      byteStride(0)
{
  // TODO: is this pad out by 16 still needed?
  view = make_buffer_shared_unique<char>(device.getIspcrtContext(),
      size() * sizeOf(type) + 16,
      ispcrt::SharedMemoryUsageHint::HostWriteDeviceRead);
  addr = view->data();

  init();
  if (isObjectType(type)) // XXX initialize always? or never?
    memset(addr, 0, size() * sizeOf(type));
}

Data::~Data()
{
  if (isObjectType(type)) {
    for (auto &&child : as<ManagedObject *, 3>()) {
      if (child)
        child->refDec();
    }
  }
}

ispc::Data1D Data::emptyData1D;

void Data::init()
{
  managedObjectType = OSP_DATA;
  if (reduce_min(numItems) == 0)
    throw std::out_of_range("OSPData: all numItems must be positive");
  dimensions =
      std::max(1, (numItems.x > 1) + (numItems.y > 1) + (numItems.z > 1));
  // compute strides if requested
  if (byteStride.x == 0)
    byteStride.x = sizeOf(type);
  if (byteStride.y == 0)
    byteStride.y = numItems.x * byteStride.x;
  if (byteStride.z == 0)
    byteStride.z = numItems.y * byteStride.y;

#ifdef OSPRAY_TARGET_SYCL
  // Check if the shared data the app gave is actually in USM, if not we still
  // need to make a copy of it internally so it's accessible on the GPU
  if (shared) {
    ispcrt::Context &ispcrtContext = getISPCDevice().getIspcrtContext();
    ispcrt::Device &ispcrtDevice = getISPCDevice().getIspcrtDevice();
    auto memType = ispcrtDevice.getMemoryAllocType(addr);

    if (memType != ISPCRT_ALLOC_TYPE_SHARED) {
      const size_t sizeBytes = byteStride.z * numItems.z;
      shared = false;
      // TODO: is the padding still needed?
      view = make_buffer_shared_unique<char>(ispcrtContext,
          sizeBytes + 16,
          ispcrt::SharedMemoryUsageHint::HostWriteDeviceRead);
      addr = view->data();
      std::memcpy(addr, appSharedPtr, sizeBytes);
    }
  }
#endif

  // precompute dominant axis and set at ispc-side proxy
  if (dimensions != 1)
    return;

  ispc.byteStride = byteStride.x;
  size_t numItems1D = numItems.x;
  if (numItems.y > 1) {
    ispc.byteStride = byteStride.y;
    numItems1D = numItems.y;
  } else if (numItems.z > 1) {
    ispc.byteStride = byteStride.z;
    numItems1D = numItems.z;
  }
  // finalize ispc-side
  ispc.addr = reinterpret_cast<decltype(ispc.addr)>(addr);
  ispc.huge = std::abs(ispc.byteStride) * numItems1D
      > (size_t)std::numeric_limits<int32_t>::max();
  ispc.numItems = (uint32_t)numItems1D;
}

bool Data::compact() const
{
  return data() + sizeOf(type) * (size() - 1) == data(numItems - 1);
}

void Data::copy(const Data &source, const vec3ul &destinationIndex)
{
  if (type != source.type
      && !(type == OSP_OBJECT && isObjectType(source.type))) {
    throw std::runtime_error(toString()
        + "::copy: types must match (cannot copy '" + stringFor(source.type)
        + "' into '" + stringFor(type) + "')");
  }

  if (shared && !source.shared) {
    throw std::runtime_error(
        "OSPData::copy: cannot copy opaque (non-shared) data into shared "
        "data");
  }

  if (anyLessThan(numItems, destinationIndex + source.numItems)) {
    throw std::out_of_range(
        "OSPData::copy: source does not fit into destination");
  }

  if (byteStride == source.byteStride
      && data(destinationIndex) == source.data()) {
    // NoOP, no need to copy identical region
    // TODO markDirty(destinationIndex, destinationIndex + source.numItems);
    return;
  }

  index_sequence_3D srcIndices(source.numItems);

  for (auto srcIdx : srcIndices) {
    char *dst = data(srcIdx + destinationIndex);
    const char *src = source.data(srcIdx);
    if (isObjectType(type)) {
      const ManagedObject **srcO = (const ManagedObject **)src;
      if (*srcO)
        (*srcO)->refInc();
      const ManagedObject **dstO = (const ManagedObject **)dst;
      if (*dstO)
        (*dstO)->refDec();
      *dstO = *srcO;
    } else {
      memcpy(dst, src, sizeOf(type));
    }
  }
}

#ifdef OSPRAY_TARGET_SYCL
void Data::commit()
{
  // If we were passed "shared" data that was not actually in USM we made a USM
  // copy of it, and need to update that copy on commit
  if (appSharedPtr && addr != appSharedPtr) {
    const size_t sizeBytes = byteStride.z * numItems.z;
    std::memcpy(addr, appSharedPtr, sizeBytes);
  }
}
#endif

bool Data::isShared() const
{
  return shared;
}

std::string Data::toString() const
{
  return "ospray::Data";
}

OSPTYPEFOR_DEFINITION(Data *);

} // namespace ospray
