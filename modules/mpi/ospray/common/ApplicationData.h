// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <rkcommon/memory/IntrusivePtr.h>
#include <memory>
#include "ospray/OSPEnums.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/ArrayView.h"

namespace ospray {
namespace mpi {

using namespace rkcommon;
using namespace rkcommon::math;

/* The data shared with the application. The workerType may not be the same
 * as the type of the data, because managed objects are unknown on the
 * application rank, and are represented just as uint64 handles. Thus the
 * local data objects see them as OSP_ULONG types, while the worker type
 * is the true managed object type
 */
struct ApplicationData : memory::RefCountedObject
{
  std::shared_ptr<utility::ArrayView<uint8_t>> sharedData;
  OSPDataType type = OSP_UNKNOWN;
  vec3ul numItems = 0;
  vec3l byteStride = 0;

  bool releaseHazard = false;

  ApplicationData() = default;

  ApplicationData(const void *sharedData,
      OSPDataType type,
      const vec3ul &numItems,
      const vec3l &byteStride);

  uint8_t *data(const vec3ul &idx) const;

  size_t size() const;

  bool compact() const;

  // Copy/Compact the data to the output dst passed. If the data is already
  // compact this is equivalent to a memcpy.
  void compactTo(void *dst) const;
};

inline uint8_t *ApplicationData::data(const vec3ul &idx) const
{
  return sharedData->data() + idx.x * byteStride.x + idx.y * byteStride.y
      + idx.z * byteStride.z;
}

inline size_t ApplicationData::size() const
{
  return numItems.long_product();
}

} // namespace mpi
} // namespace ospray
