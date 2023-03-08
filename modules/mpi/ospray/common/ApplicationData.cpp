// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ApplicationData.h"
#include <cstring>
#include <memory>
#include "MPICommon.h"
#include "common/OSPCommon.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/multidim_index_sequence.h"

namespace ospray {
namespace mpi {
ApplicationData::ApplicationData(const void *appData,
    OSPDataType type,
    const vec3ul &nItems,
    const vec3l &bStride)
    : type(type), numItems(nItems), byteStride(bStride)
{
  // compute strides if requested
  if (byteStride.x == 0)
    byteStride.x = sizeOf(type);
  if (byteStride.y == 0)
    byteStride.y = numItems.x * byteStride.x;
  if (byteStride.z == 0)
    byteStride.z = numItems.y * byteStride.y;

  sharedData = std::make_shared<utility::ArrayView<uint8_t>>(
      const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(appData)),
      byteStride.z * numItems.z);
}

bool ApplicationData::compact() const
{
  // Compare the last offset using the byte stride vs. the last offset we'd have
  // if using a compact stride
  return sharedData->data() + sizeOf(type) * (size() - 1) == data(numItems - 1);
}

// Copy/Compact the data to the output dst passed. If the data is already
// compact this is equivalent to a memcpy.
void ApplicationData::compactTo(void *dst) const
{
  if (compact()) {
    std::memcpy(dst, sharedData->data(), sharedData->size());
  } else {
    // Probably ok with assuming sizeof(void*) == 8b ? I don't think we support
    // 32bit builds?
    const size_t typeSize =
        mpicommon::isManagedObject(type) ? sizeof(int64_t) : sizeOf(type);
    uint8_t *out = reinterpret_cast<uint8_t *>(dst);
    index_sequence_3D srcIndices(numItems);
    size_t destIdx = 0;
    for (auto srcIdx : srcIndices) {
      const uint8_t *src = data(srcIdx);
      std::memcpy(out + destIdx * typeSize, src, typeSize);
      ++destIdx;
    }
  }
}

} // namespace mpi
} // namespace ospray
