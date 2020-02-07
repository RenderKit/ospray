// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospray.h"

#ifdef __cplusplus
extern "C" {
#endif

// OSPData helpers //////////////////////////////////////////////////////////

OSPRAY_INTERFACE OSPData ospNewSharedData1D(
    const void *sharedData, OSPDataType type, uint64_t numItems);

OSPRAY_INTERFACE OSPData ospNewSharedData1DStride(const void *sharedData,
    OSPDataType type,
    uint64_t numItems,
    int64_t byteStride);

OSPRAY_INTERFACE OSPData ospNewSharedData2D(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    uint64_t numItems2);

OSPRAY_INTERFACE OSPData ospNewSharedData2DStride(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    int64_t byteStride1,
    uint64_t numItems2,
    int64_t byteStride2);

OSPRAY_INTERFACE OSPData ospNewSharedData3D(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    uint64_t numItems2,
    uint64_t numItems3);

OSPRAY_INTERFACE OSPData ospNewData1D(OSPDataType type, uint64_t numItems);

OSPRAY_INTERFACE OSPData ospNewData2D(
    OSPDataType type, uint64_t numItems1, uint64_t numItems2);

OSPRAY_INTERFACE void ospCopyData1D(
    const OSPData source, OSPData destination, uint64_t destinationIndex);

OSPRAY_INTERFACE void ospCopyData2D(const OSPData source,
    OSPData destination,
    uint64_t destinationIndex1,
    uint64_t destinationIndex2);

// Parameter helpers //////////////////////////////////////////////////////////

OSPRAY_INTERFACE void ospSetString(OSPObject, const char *n, const char *s);
OSPRAY_INTERFACE void ospSetObject(OSPObject, const char *n, OSPObject obj);

OSPRAY_INTERFACE void ospSetBool(OSPObject, const char *n, int x);
OSPRAY_INTERFACE void ospSetFloat(OSPObject, const char *n, float x);
OSPRAY_INTERFACE void ospSetInt(OSPObject, const char *n, int x);

// clang-format off
OSPRAY_INTERFACE void ospSetVec2f(OSPObject, const char *n, float x, float y);
OSPRAY_INTERFACE void ospSetVec3f(OSPObject, const char *n, float x, float y, float z);
OSPRAY_INTERFACE void ospSetVec4f(OSPObject, const char *n, float x, float y, float z, float w);

OSPRAY_INTERFACE void ospSetVec2i(OSPObject, const char *n, int x, int y);
OSPRAY_INTERFACE void ospSetVec3i(OSPObject, const char *n, int x, int y, int z);
OSPRAY_INTERFACE void ospSetVec4i(OSPObject, const char *n, int x, int y, int z, int w);

// Take 'obj' and put it in an opaque OSPData array with given element type, then set on 'target'
OSPRAY_INTERFACE void ospSetObjectAsData(OSPObject target,
                                         const char *n,
                                         OSPDataType type,
                                         OSPObject obj);
// clang-format on

// Rendering helpers //////////////////////////////////////////////////////////

// Start a frame task and immediately wait on it, return frame buffer varaince
OSPRAY_INTERFACE float ospRenderFrameBlocking(
    OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld);

#ifdef __cplusplus
} // extern "C"
#endif
