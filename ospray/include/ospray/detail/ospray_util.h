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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

  static inline OSPData ospNewSharedData1D(
      const void *sharedData, OSPDataType type, uint32_t numItems)
  {
    return ospNewSharedData(sharedData, type, numItems, 0, 1, 0, 1, 0);
  }

  static inline OSPData ospNewSharedData1DStride(const void *sharedData,
      OSPDataType type,
      uint32_t numItems,
      int64_t byteStride)
  {
    return ospNewSharedData(sharedData, type, numItems, byteStride, 1, 0, 1, 0);
  }

  static inline OSPData ospNewSharedData2D(const void *sharedData,
      OSPDataType type,
      uint32_t numItems1,
      uint32_t numItems2)
  {
    return ospNewSharedData(sharedData, type, numItems1, 0, numItems2, 0, 1, 0);
  }

  static inline OSPData ospNewSharedData2DStride(const void *sharedData,
      OSPDataType type,
      uint32_t numItems1,
      int64_t byteStride1,
      uint32_t numItems2,
      int64_t byteStride2)
  {
    return ospNewSharedData(
        sharedData, type, numItems1, byteStride1, numItems2, byteStride2, 1, 0);
  }

  static inline OSPData ospNewSharedData3D(const void *sharedData,
      OSPDataType type,
      uint32_t numItems1,
      uint32_t numItems2,
      uint32_t numItems3)
  {
    return ospNewSharedData(
        sharedData, type, numItems1, 0, numItems2, 0, numItems3, 0);
  }

  static inline OSPData ospNewData1D(OSPDataType type, uint32_t numItems)
  {
    return ospNewData(type, numItems, 1, 1);
  }

  static inline OSPData ospNewData2D(
      OSPDataType type, uint32_t numItems1, uint32_t numItems2)
  {
    return ospNewData(type, numItems1, numItems2, 1);
  }

  static inline void ospCopyData1D(
      const OSPData source, OSPData destination, uint32_t destinationIndex)
  {
    ospCopyData(source, destination, destinationIndex, 0, 0);
  }

  static inline void ospCopyData2D(const OSPData source,
      OSPData destination,
      uint32_t destinationIndex1,
      uint32_t destinationIndex2)
  {
    ospCopyData(source, destination, destinationIndex1, destinationIndex2, 0);
  }

#ifdef __cplusplus
} // extern "C"

// XXX temporary, to maintain backwards compatibility
static inline OSPData ospNewData(size_t numItems,
    OSPDataType type,
    const void *source,
    uint32_t dataCreationFlags OSP_DEFAULT_VAL(0))
{
  if (dataCreationFlags)
    return ospNewSharedData(source, type, numItems);
  else {
    OSPData src = ospNewSharedData(source, type, numItems);
    OSPData dst = ospNewData(type, numItems);
    ospCopyData(src, dst);
    ospRelease(src);
    return dst;
  }
}
#endif
