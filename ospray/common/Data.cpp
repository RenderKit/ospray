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

namespace ospray {

  Data::Data(const void *sharedData,
      OSPDataType type,
      const vec3ui &numItems,
      const vec3l &byteStride)
      : shared(true), type(type), numItems(numItems), byteStride(byteStride)

  {
    if (sharedData == nullptr)
      throw std::runtime_error("OSPData: shared buffer is NULL");
    init();
    addr = (char *)sharedData;

    if (isObjectType(type)) {
      ManagedObject **child = (ManagedObject **)data();
      for (uint32_t i = 0; i < numItems.x; i++)
        if (child[i])
          child[i]->refInc();
    }
  }

  Data::Data(OSPDataType type, const vec3ui &numItems)
      : shared(false), type(type), numItems(numItems), byteStride(0)
  {
    init();
    addr = (char *)alignedMalloc(
        size() * sizeOf(type) + 16); // XXX padding needed?
    if (isObjectType(type)) // XXX initialize always? or never?
      memset(addr, 0, size() * sizeOf(type));
  }

  Data::~Data()
  {
    if (isObjectType(type)) {
      ManagedObject **child = (ManagedObject **)data();
      for (uint32_t i = 0; i < numItems.x; i++)
        if (child[i])
          child[i]->refDec();
    }

    if (!shared)
      alignedFree(addr);
  }

  void Data::init()
  {
    numBytes = size() * sizeOf(type);
    managedObjectType = OSP_DATA;
    if (reduce_min(numItems) == 0)
      throw std::out_of_range("OSPData: all numItems must be positive");
    dimensions = (numItems.x > 1) + (numItems.y > 1) + (numItems.z > 1);
    // compute strides if requested
    if (byteStride.x == 0)
      byteStride.x = sizeOf(type);
    if (byteStride.y == 0)
      byteStride.y = numItems.x * sizeOf(type);
    if (byteStride.z == 0)
      byteStride.z = numItems.x * numItems.y * sizeOf(type);
  }

  void Data::copy(const Data &source, const vec3ui &destinationIndex)
  {
    if (type != source.type
        && !(isObjectType(type) && isObjectType(source.type)))
      throw std::runtime_error("OSPData::copy: types must match");
    if (shared && !source.shared)
      throw std::runtime_error(
          "OSPData::copy: cannot copy opaque (non-shared) data into shared data");
    if (anyLessThan(numItems, destinationIndex + source.numItems))
      throw std::out_of_range(
          "OSPData::copy: source does not fit into destination");

    if (byteStride == source.byteStride
        && data(destinationIndex) == source.data()) {
      // NoOP, no need to copy identical region
      // TODO markDirty(destinationIndex, destinationIndex + source.numItems);
      return;
    }

    vec3ui srcIdx;
    for (srcIdx.z = 0; srcIdx.z < source.numItems.z; srcIdx.z++)
      for (srcIdx.y = 0; srcIdx.y < source.numItems.y; srcIdx.y++)
        for (srcIdx.x = 0; srcIdx.x < source.numItems.x; srcIdx.x++) {
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
          } else
            memcpy(dst, src, sizeOf(type));
        }
  }

  std::string Data::toString() const
  {
    return "ospray::Data";
  }

} // ::ospray
