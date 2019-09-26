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

#include "ospray.h"

#ifdef __cplusplus
extern "C" {
#endif

// OSPData helpers //////////////////////////////////////////////////////////

OSPData ospNewSharedData1D(const void *sharedData,
                           OSPDataType type,
                           uint32_t numItems);

OSPData ospNewSharedData1DStride(const void *sharedData,
                                 OSPDataType type,
                                 uint32_t numItems,
                                 int64_t byteStride);

OSPData ospNewSharedData2D(const void *sharedData,
                           OSPDataType type,
                           uint32_t numItems1,
                           uint32_t numItems2);

OSPData ospNewSharedData2DStride(const void *sharedData,
                                 OSPDataType type,
                                 uint32_t numItems1,
                                 int64_t byteStride1,
                                 uint32_t numItems2,
                                 int64_t byteStride2);

OSPData ospNewSharedData3D(const void *sharedData,
                           OSPDataType type,
                           uint32_t numItems1,
                           uint32_t numItems2,
                           uint32_t numItems3);

OSPData ospNewData1D(OSPDataType type, uint32_t numItems);

OSPData ospNewData2D(OSPDataType type, uint32_t numItems1, uint32_t numItems2);

void ospCopyData1D(const OSPData source,
                   OSPData destination,
                   uint32_t destinationIndex);

void ospCopyData2D(const OSPData source,
                   OSPData destination,
                   uint32_t destinationIndex1,
                   uint32_t destinationIndex2);

// Parameter helpers //////////////////////////////////////////////////////////

void ospSetString(OSPObject, const char *n, const char *s);
void ospSetObject(OSPObject, const char *n, OSPObject obj);
void ospSetData(OSPObject, const char *n, OSPData data);

void ospSetBool(OSPObject, const char *n, int x);
void ospSetFloat(OSPObject, const char *n, float x);
void ospSetInt(OSPObject, const char *n, int x);

void ospSetVec2f(OSPObject, const char *n, float x, float y);
void ospSetVec3f(OSPObject, const char *n, float x, float y, float z);
void ospSetVec4f(OSPObject, const char *n, float x, float y, float z, float w);

void ospSetVec2i(OSPObject, const char *n, int x, int y);
void ospSetVec3i(OSPObject, const char *n, int x, int y, int z);
void ospSetVec4i(OSPObject, const char *n, int x, int y, int z, int w);

#ifdef __cplusplus
}  // extern "C"

// XXX temporary, to maintain backwards compatibility
OSPData ospNewData(size_t numItems,
                   OSPDataType type,
                   const void *source,
                   uint32_t dataCreationFlags OSP_DEFAULT_VAL(0));
#endif
