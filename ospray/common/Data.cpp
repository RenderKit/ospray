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
#include "Data.h"
#include "ospray/ospray.h"
// ospcommon
#include "ospcommon/utility/multidim_index_sequence.h"

namespace ospray {

  Data::Data(const void *sharedData,
             OSPDataType type,
             const vec3ui &numItems,
             const vec3l &byteStride)
      : shared(true), type(type), numItems(numItems), byteStride(byteStride)

  {
    if (sharedData == nullptr)
      throw std::runtime_error("OSPData: shared buffer is NULL");
    addr = (char *)sharedData;
    init();

    if (isObjectType(type)) {
      for (auto &&child : as<ManagedObject *, 3>()) {
        if (child)
          child->refInc();
      }
    }
  }

  Data::Data(OSPDataType type, const vec3ui &numItems)
      : shared(false), type(type), numItems(numItems), byteStride(0)
  {
    addr = (char *)alignedMalloc(size() * sizeOf(type) +
                                 16);  // XXX padding needed?
    init();
    if (isObjectType(type))  // XXX initialize always? or never?
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

    if (!shared)
      alignedFree(addr);
  }

  ispc::Data1D Data::empytData1D;

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
      byteStride.y = numItems.x * sizeOf(type);
    if (byteStride.z == 0)
      byteStride.z = numItems.x * numItems.y * sizeOf(type);

    // precompute dominant axis and set at ispc-side proxy
    if (dimensions != 1)
      return;

    ispc.byteStride = byteStride.x;
    ispc.numItems   = numItems.x;
    if (numItems.y > 1) {
      ispc.byteStride = byteStride.y;
      ispc.numItems   = numItems.y;
    } else if (numItems.z > 1) {
      ispc.byteStride = byteStride.z;
      ispc.numItems   = numItems.z;
    }
    // finalize ispc-side
    ispc.addr = reinterpret_cast<decltype(ispc.addr)>(addr);
    ispc.huge = ispc.byteStride * ispc.numItems >
                std::numeric_limits<std::int32_t>::max();
  }

  bool Data::compact() const
  {
    return data() + sizeOf(type) * (size() - 1) == data(numItems - 1);
  }

  void Data::copy(const Data &source, const vec3ui &destinationIndex)
  {
    if (type != source.type &&
        !(type == OSP_OBJECT && isObjectType(source.type))) {
      throw std::runtime_error(
          toString() + "::copy: types must match (cannot copy '" +
          stringFor(source.type) + "' into '" + stringFor(type) + "')");
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

    if (byteStride == source.byteStride &&
        data(destinationIndex) == source.data()) {
      // NoOP, no need to copy identical region
      // TODO markDirty(destinationIndex, destinationIndex + source.numItems);
      return;
    }

    index_sequence_3D srcIndices(source.numItems);

    for (auto srcIdx : srcIndices) {
      char *dst       = data(srcIdx + destinationIndex);
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

  std::string Data::toString() const
  {
    return "ospray::Data";
  }

}  // namespace ospray
