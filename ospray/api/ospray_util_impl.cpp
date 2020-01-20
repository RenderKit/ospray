// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospray/ospray_cpp.h"
#include "ospray/ospray_util.h"

extern "C" {

// OSPData helpers ////////////////////////////////////////////////////////////

OSPData ospNewSharedData1D(
    const void *sharedData, OSPDataType type, uint64_t numItems)
{
  return ospNewSharedData(sharedData, type, numItems, 0, 1, 0, 1, 0);
}

OSPData ospNewSharedData1DStride(const void *sharedData,
    OSPDataType type,
    uint64_t numItems,
    int64_t byteStride)
{
  return ospNewSharedData(sharedData, type, numItems, byteStride, 1, 0, 1, 0);
}

OSPData ospNewSharedData2D(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    uint64_t numItems2)
{
  return ospNewSharedData(sharedData, type, numItems1, 0, numItems2, 0, 1, 0);
}

OSPData ospNewSharedData2DStride(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    int64_t byteStride1,
    uint64_t numItems2,
    int64_t byteStride2)
{
  return ospNewSharedData(
      sharedData, type, numItems1, byteStride1, numItems2, byteStride2, 1, 0);
}

OSPData ospNewSharedData3D(const void *sharedData,
    OSPDataType type,
    uint64_t numItems1,
    uint64_t numItems2,
    uint64_t numItems3)
{
  return ospNewSharedData(
      sharedData, type, numItems1, 0, numItems2, 0, numItems3, 0);
}

OSPData ospNewData1D(OSPDataType type, uint64_t numItems)
{
  return ospNewData(type, numItems, 1, 1);
}

OSPData ospNewData2D(OSPDataType type, uint64_t numItems1, uint64_t numItems2)
{
  return ospNewData(type, numItems1, numItems2, 1);
}

void ospCopyData1D(
    const OSPData source, OSPData destination, uint64_t destinationIndex)
{
  ospCopyData(source, destination, destinationIndex, 0, 0);
}

void ospCopyData2D(const OSPData source,
    OSPData destination,
    uint64_t destinationIndex1,
    uint64_t destinationIndex2)
{
  ospCopyData(source, destination, destinationIndex1, destinationIndex2, 0);
}

// Parameter helpers ////////////////////////////////////////////////////////

void ospSetString(OSPObject o, const char *id, const char *s)
{
  ospSetParam(o, id, OSP_STRING, &s);
}

void ospSetObject(OSPObject o, const char *id, OSPObject other)
{
  ospSetParam(o, id, OSP_OBJECT, &other);
}

void ospSetBool(OSPObject o, const char *id, int x)
{
  ospSetParam(o, id, OSP_BOOL, &x);
}

void ospSetFloat(OSPObject o, const char *id, float x)
{
  ospSetParam(o, id, OSP_FLOAT, &x);
}

void ospSetInt(OSPObject o, const char *id, int x)
{
  ospSetParam(o, id, OSP_INT, &x);
}

void ospSetVec2f(OSPObject o, const char *id, float x, float y)
{
  float v[] = {x, y};
  ospSetParam(o, id, OSP_VEC2F, v);
}

void ospSetVec3f(OSPObject o, const char *id, float x, float y, float z)
{
  float v[] = {x, y, z};
  ospSetParam(o, id, OSP_VEC3F, v);
}

void ospSetVec4f(
    OSPObject o, const char *id, float x, float y, float z, float w)
{
  float v[] = {x, y, z, w};
  ospSetParam(o, id, OSP_VEC4F, v);
}

void ospSetVec2i(OSPObject o, const char *id, int x, int y)
{
  int v[] = {x, y};
  ospSetParam(o, id, OSP_VEC2I, v);
}

void ospSetVec3i(OSPObject o, const char *id, int x, int y, int z)
{
  int v[] = {x, y, z};
  ospSetParam(o, id, OSP_VEC3I, v);
}

void ospSetVec4i(OSPObject o, const char *id, int x, int y, int z, int w)
{
  int v[] = {x, y, z, w};
  ospSetParam(o, id, OSP_VEC4I, v);
}

void ospSetObjectAsData(
    OSPObject o, const char *n, OSPDataType type, OSPObject p)
{
  OSPData tmp = ospNewSharedData(&p, type, 1);
  OSPData data = ospNewData(type, 1);
  ospCopyData(tmp, data);
  ospSetParam(o, n, OSP_DATA, &data);
  ospRelease(tmp);
  ospRelease(data);
}

// Rendering helpers //////////////////////////////////////////////////////////

float ospRenderFrameBlocking(
    OSPFrameBuffer fb, OSPRenderer renderer, OSPCamera camera, OSPWorld world)
{
  OSPFuture f = ospRenderFrame(fb, renderer, camera, world);
  ospWait(f, OSP_TASK_FINISHED);
  ospRelease(f);
  return ospGetVariance(fb);
}

} // extern "C"
