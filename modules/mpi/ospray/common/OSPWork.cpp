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

#include <vector>

#include "../fb/DistributedFrameBuffer.h"
#include "MPICommon.h"
#include "OSPWork.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/ObjectHandle.h"
#include "common/World.h"
#include "geometry/GeometricModel.h"
#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/OwnedArray.h"
#include "ospcommon/utility/multidim_index_sequence.h"
#include "render/RenderTask.h"
#include "texture/Texture.h"
#include "volume/VolumetricModel.h"

namespace ospray {
namespace mpi {
namespace work {

FrameBufferInfo::FrameBufferInfo(
    const vec2i &size, OSPFrameBufferFormat format, uint32_t channels)
    : size(size), format(format), channels(channels)
{}

size_t FrameBufferInfo::pixelSize(uint32_t channel) const
{
  switch (channel) {
  case OSP_FB_COLOR:
    switch (format) {
    case OSP_FB_RGBA8:
    case OSP_FB_SRGBA:
      return sizeof(uint32_t);
    case OSP_FB_RGBA32F:
      return sizeof(vec4f);
    default:
      return 0;
    }
  case OSP_FB_DEPTH:
    return channels & OSP_FB_DEPTH ? sizeof(float) : 0;
  case OSP_FB_NORMAL:
    return channels & OSP_FB_NORMAL ? sizeof(vec3f) : 0;
  case OSP_FB_ALBEDO:
    return channels & OSP_FB_ALBEDO ? sizeof(vec3f) : 0;
  default:
    return 0;
  }
}

size_t FrameBufferInfo::getNumPixels() const
{
  return size.x * size.y;
}

void newRenderer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewRenderer(type.c_str());
}

void newWorld(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  state.objects[handle] = ospNewWorld();
}

void newGeometry(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewGeometry(type.c_str());
}

void newGeometricModel(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t geomHandle = 0;
  cmdBuf >> handle >> geomHandle;
  state.objects[handle] =
      ospNewGeometricModel(state.getObject<OSPGeometry>(geomHandle));
}

void newVolume(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewVolume(type.c_str());
}

void newVolumetricModel(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t volHandle = 0;
  cmdBuf >> handle >> volHandle;
  state.objects[handle] =
      ospNewVolumetricModel(state.getObject<OSPVolume>(volHandle));
}

void newCamera(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewCamera(type.c_str());
}

void newTransferFunction(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewTransferFunction(type.c_str());
}

void newImageOperation(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewImageOperation(type.c_str());
}

void newMaterial(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type, rendererType;
  cmdBuf >> handle >> rendererType >> type;
  state.objects[handle] = ospNewMaterial(rendererType.c_str(), type.c_str());
}

void newLight(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewLight(type.c_str());
}

void newSharedData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  using namespace utility;

  int64_t handle = 0;
  OSPDataType format;
  vec3i numItems = 0;
  vec3l byteStride;
  cmdBuf >> handle >> format >> numItems >> byteStride;

  vec3l stride = byteStride;
  if (stride.x == 0) {
    stride.x = sizeOf(format);
  }
  if (stride.y == 0) {
    stride.y = numItems.x * sizeOf(format);
  }
  if (stride.z == 0) {
    stride.z = numItems.x * numItems.y * sizeOf(format);
  }

  size_t nbytes = numItems.x * stride.x;
  if (numItems.y > 1) {
    nbytes += numItems.y * stride.y;
  }
  if (numItems.z > 1) {
    nbytes += numItems.z * stride.z;
  }

  auto view = ospcommon::make_unique<OwnedArray<uint8_t>>();
  view->resize(nbytes, 0);
  fabric.recvBcast(*view);

  // If the data type is managed we need to convert the handles
  // back into pointers
  if (mpicommon::isManagedObject(format)) {
    ospcommon::index_sequence_3D indices(numItems);
    for (auto idx : indices) {
      const size_t i = idx.x * stride.x + idx.y * stride.y + idx.z * stride.z;
      uint8_t *addr = view->data() + i;
      int64_t *handle = reinterpret_cast<int64_t *>(addr);
      OSPObject *obj = reinterpret_cast<OSPObject *>(addr);
      *obj = state.objects[*handle];
    }
  }

  state.objects[handle] = ospNewSharedData(view->data(),
      format,
      numItems.x,
      byteStride.x,
      numItems.y,
      byteStride.y,
      numItems.z,
      byteStride.z);

  // Offload device keeps +1 ref for tracking the lifetime of the data
  // buffer shared with OSPRay
  ospRetain(state.objects[handle]);

  state.data[handle] = std::move(view);
}

void newData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  OSPDataType format;
  vec3i numItems = 0;
  cmdBuf >> handle >> format >> numItems;

  state.objects[handle] =
      ospNewData(format, numItems.x, numItems.y, numItems.z);
}

void copyData(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t sourceHandle = 0;
  int64_t destinationHandle = 0;
  vec3i destinationIndex = 0;
  cmdBuf >> sourceHandle >> destinationHandle >> destinationIndex;

  ospCopyData(state.getObject<OSPData>(sourceHandle),
      state.getObject<OSPData>(destinationHandle),
      destinationIndex.x,
      destinationIndex.y,
      destinationIndex.z);
}

void newTexture(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string type;
  cmdBuf >> handle >> type;
  state.objects[handle] = ospNewTexture(type.c_str());
}

void newGroup(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  state.objects[handle] = ospNewGroup();
}

void newInstance(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  int64_t groupHandle = 0;
  cmdBuf >> handle >> groupHandle;
  state.objects[handle] =
      ospNewInstance(state.getObject<OSPGroup>(groupHandle));
}

void commit(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;

  // If it's a data being committed, we need to recieve the updated data
  auto d = state.data.find(handle);
  if (d != state.data.end()) {
    fabric.recvBcast(*d->second);
  }

  ospCommit(state.objects[handle]);
}

void release(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;

  ospRelease(state.objects[handle]);

  auto f = state.framebuffers.find(handle);
  if (f != state.framebuffers.end()) {
    state.framebuffers.erase(f);
  }

  // Pass through the data and see if any have been completely free'd
  // i.e., no object depends on them anymore either.
  for (auto it = state.data.begin(); it != state.data.end();) {
    OSPObject obj = state.objects[it->first];
    ManagedObject *m = reinterpret_cast<ManagedObject *>(obj);
    if (m->useCount() == 1) {
      ospRelease(state.objects[it->first]);
      it = state.data.erase(it);
    } else {
      ++it;
    }
  }
}

void retain(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;

  ospRetain(state.objects[handle]);
}

void loadModule(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  std::string module;
  cmdBuf >> module;
  ospLoadModule(module.c_str());
}

void createFramebuffer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  vec2i size(0, 0);
  uint32_t format;
  uint32_t channels = 0;
  cmdBuf >> handle >> size >> format >> channels;
  state.objects[handle] =
      ospNewFrameBuffer(size.x, size.y, (OSPFrameBufferFormat)format, channels);
  state.framebuffers[handle] =
      FrameBufferInfo(size, (OSPFrameBufferFormat)format, channels);
}

void mapFramebuffer(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  // Map the channel and send the image back over the fabric
  if (mpicommon::worker.rank == 0) {
    using namespace utility;

    int64_t handle = 0;
    uint32_t channel = 0;
    cmdBuf >> handle >> channel;

    const FrameBufferInfo &fbInfo = state.framebuffers[handle];
    uint64_t nbytes = fbInfo.pixelSize(channel) * fbInfo.getNumPixels();

    auto bytesView = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&nbytes), sizeof(nbytes));
    fabric.send(bytesView, 0);

    if (nbytes != 0) {
      OSPFrameBuffer fb = state.getObject<OSPFrameBuffer>(handle);
      void *map = const_cast<void *>(
          ospMapFrameBuffer(fb, (OSPFrameBufferChannel)channel));

      auto fbView = std::make_shared<OwnedArray<uint8_t>>(
          reinterpret_cast<uint8_t *>(map), nbytes);

      fabric.send(fbView, 0);
      ospUnmapFrameBuffer(map, fb);
    }
  }
}

void getVariance(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  // Map the channel and send the image back over the fabric
  if (mpicommon::worker.rank == 0) {
    using namespace utility;

    int64_t handle = 0;
    cmdBuf >> handle;

    float variance = ospGetVariance(state.getObject<OSPFrameBuffer>(handle));

    auto bytesView = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&variance), sizeof(variance));
    fabric.send(bytesView, 0);
  }
}

void resetAccumulation(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospResetAccumulation(state.getObject<OSPFrameBuffer>(handle));
}

void renderFrameAsync(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t futureHandle = 0;
  int64_t fbHandle = 0;
  int64_t rendererHandle = 0;
  int64_t cameraHandle = 0;
  int64_t worldHandle = 0;
  cmdBuf >> fbHandle >> rendererHandle >> cameraHandle >> worldHandle
      >> futureHandle;
  state.objects[futureHandle] =
      ospRenderFrameAsync(state.getObject<OSPFrameBuffer>(fbHandle),
          state.getObject<OSPRenderer>(rendererHandle),
          state.getObject<OSPCamera>(cameraHandle),
          state.getObject<OSPWorld>(worldHandle));
}

template <typename T>
void setParam(networking::BufferReader &cmdBuf,
    OSPObject obj,
    const std::string &param,
    OSPDataType type)
{
  T val;
  cmdBuf >> val;
  ospSetParam(obj, param.c_str(), type, &val);
}

void setParam(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string param;
  OSPDataType type;
  cmdBuf >> handle >> param >> type;

  switch (type) {
  // All OSP_OBJECT fall through the same style of setting param since it's just
  // a handle
  case OSP_DEVICE:
  case OSP_OBJECT:
  case OSP_CAMERA:
  case OSP_DATA:
  case OSP_FRAMEBUFFER:
  case OSP_FUTURE:
  case OSP_GEOMETRIC_MODEL:
  case OSP_GEOMETRY:
  case OSP_GROUP:
  case OSP_IMAGE_OPERATION:
  case OSP_INSTANCE:
  case OSP_LIGHT:
  case OSP_MATERIAL:
  case OSP_RENDERER:
  case OSP_TEXTURE:
  case OSP_TRANSFER_FUNCTION:
  case OSP_VOLUME:
  case OSP_VOLUMETRIC_MODEL:
  case OSP_WORLD: {
    int64_t val = 0;
    cmdBuf >> val;
    ospSetParam(
        state.objects[handle], param.c_str(), type, &state.objects[val]);
    break;
  }
  case OSP_STRING: {
    std::string val = 0;
    cmdBuf >> val;
    ospSetParam(state.objects[handle], param.c_str(), type, val.c_str());
    break;
  }
  case OSP_BOOL:
    setParam<bool>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_CHAR:
  case OSP_BYTE:
    setParam<char>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2UC:
    setParam<vec2uc>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3UC:
    setParam<vec3uc>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4UC:
    setParam<vec4uc>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_SHORT:
    setParam<short>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_USHORT:
    setParam<unsigned short>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_INT:
    setParam<int>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2I:
    setParam<vec2i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3I:
    setParam<vec3i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4I:
    setParam<vec4i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_UINT:
    setParam<unsigned int>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2UI:
    setParam<vec2ui>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3UI:
    setParam<vec3ui>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4UI:
    setParam<vec4ui>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_LONG:
    setParam<long>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2L:
    setParam<vec2l>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3L:
    setParam<vec3l>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4L:
    setParam<vec4l>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_ULONG:
    setParam<unsigned long>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2UL:
    setParam<vec2ul>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3UL:
    setParam<vec3ul>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4UL:
    setParam<vec4ul>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_FLOAT:
    setParam<float>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC2F:
    setParam<vec2f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC3F:
    setParam<vec3f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_VEC4F:
    setParam<vec4f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_DOUBLE:
    setParam<double>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX1I:
    setParam<box1i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX2I:
    setParam<box2i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX3I:
    setParam<box3i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX4I:
    setParam<box4i>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX1F:
    setParam<box1f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX2F:
    setParam<box2f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX3F:
    setParam<box3f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_BOX4F:
    setParam<box4f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_LINEAR2F:
    setParam<linear2f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_LINEAR3F:
    setParam<linear3f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_AFFINE2F:
    setParam<affine2f>(cmdBuf, state.objects[handle], param, type);
    break;
  case OSP_AFFINE3F:
    setParam<affine3f>(cmdBuf, state.objects[handle], param, type);
    break;
  default:
    throw std::runtime_error("Unrecognized param type!");
  }
}

void removeParam(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  int64_t handle = 0;
  std::string param;
  cmdBuf >> handle >> param;
  ospRemoveParam(state.objects[handle], param.c_str());
}

void setLoadBalancer(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  PING;
  NOT_IMPLEMENTED;
  // TODO: We only have one load balancer now
}

void pick(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t fbHandle = 0;
  int64_t rendererHandle = 0;
  int64_t cameraHandle = 0;
  int64_t worldHandle = 0;
  vec2f screenPos;
  cmdBuf >> fbHandle >> rendererHandle >> cameraHandle >> worldHandle
      >> screenPos;

  OSPPickResult res = {0};
  ospPick(&res,
      state.getObject<OSPFrameBuffer>(fbHandle),
      state.getObject<OSPRenderer>(rendererHandle),
      state.getObject<OSPCamera>(cameraHandle),
      state.getObject<OSPWorld>(worldHandle),
      screenPos.x,
      screenPos.y);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&res), sizeof(OSPPickResult));
    fabric.send(view, 0);
  }
}

void futureIsReady(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  uint32_t event = 0;
  cmdBuf >> handle >> event;
  int ready =
      ospIsReady(state.getObject<OSPFuture>(handle), (OSPSyncEvent)event);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&ready), sizeof(ready));
    fabric.send(view, 0);
  }
}

void futureWait(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  uint32_t event = 0;
  cmdBuf >> handle >> event;
  ospWait(state.getObject<OSPFuture>(handle), (OSPSyncEvent)event);

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&event), sizeof(event));
    fabric.send(view, 0);
  }
}

void futureCancel(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  ospCancel(state.getObject<OSPFuture>(handle));
}

void futureGetProgress(OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  int64_t handle = 0;
  cmdBuf >> handle;
  float progress = ospGetProgress(state.getObject<OSPFuture>(handle));

  if (mpicommon::worker.rank == 0) {
    using namespace utility;
    auto view = std::make_shared<OwnedArray<uint8_t>>(
        reinterpret_cast<uint8_t *>(&progress), sizeof(progress));
    fabric.send(view, 0);
  }
}

void finalize(
    OSPState &state, networking::BufferReader &cmdBuf, networking::Fabric &)
{
  maml::stop();
  MPI_Finalize();
  std::exit(0);
}

void dispatchWork(TAG t,
    OSPState &state,
    networking::BufferReader &cmdBuf,
    networking::Fabric &fabric)
{
  switch (t) {
  case NEW_RENDERER:
    newRenderer(state, cmdBuf, fabric);
    break;
  case NEW_WORLD:
    newWorld(state, cmdBuf, fabric);
    break;
  case NEW_GEOMETRY:
    newGeometry(state, cmdBuf, fabric);
    break;
  case NEW_GEOMETRIC_MODEL:
    newGeometricModel(state, cmdBuf, fabric);
    break;
  case NEW_VOLUME:
    newVolume(state, cmdBuf, fabric);
    break;
  case NEW_VOLUMETRIC_MODEL:
    newVolumetricModel(state, cmdBuf, fabric);
    break;
  case NEW_CAMERA:
    newCamera(state, cmdBuf, fabric);
    break;
  case NEW_TRANSFER_FUNCTION:
    newTransferFunction(state, cmdBuf, fabric);
    break;
  case NEW_IMAGE_OPERATION:
    newImageOperation(state, cmdBuf, fabric);
    break;
  case NEW_MATERIAL:
    newMaterial(state, cmdBuf, fabric);
    break;
  case NEW_LIGHT:
    newLight(state, cmdBuf, fabric);
    break;
  case NEW_SHARED_DATA:
    newSharedData(state, cmdBuf, fabric);
    break;
  case NEW_DATA:
    newData(state, cmdBuf, fabric);
    break;
  case COPY_DATA:
    copyData(state, cmdBuf, fabric);
    break;
  case NEW_TEXTURE:
    newTexture(state, cmdBuf, fabric);
    break;
  case NEW_GROUP:
    newGroup(state, cmdBuf, fabric);
    break;
  case NEW_INSTANCE:
    newInstance(state, cmdBuf, fabric);
    break;
  case COMMIT:
    commit(state, cmdBuf, fabric);
    break;
  case RELEASE:
    release(state, cmdBuf, fabric);
    break;
  case RETAIN:
    retain(state, cmdBuf, fabric);
    break;
  case LOAD_MODULE:
    loadModule(state, cmdBuf, fabric);
    break;
  case CREATE_FRAMEBUFFER:
    createFramebuffer(state, cmdBuf, fabric);
    break;
  case MAP_FRAMEBUFFER:
    mapFramebuffer(state, cmdBuf, fabric);
    break;
  case GET_VARIANCE:
    getVariance(state, cmdBuf, fabric);
    break;
  case RESET_ACCUMULATION:
    resetAccumulation(state, cmdBuf, fabric);
    break;
  case RENDER_FRAME_ASYNC:
    renderFrameAsync(state, cmdBuf, fabric);
    break;
  case SET_PARAM:
    setParam(state, cmdBuf, fabric);
    break;
  case REMOVE_PARAM:
    removeParam(state, cmdBuf, fabric);
    break;
  case SET_LOAD_BALANCER:
    setLoadBalancer(state, cmdBuf, fabric);
    break;
  case PICK:
    pick(state, cmdBuf, fabric);
    break;
  case FUTURE_IS_READY:
    futureIsReady(state, cmdBuf, fabric);
    break;
  case FUTURE_WAIT:
    futureWait(state, cmdBuf, fabric);
    break;
  case FUTURE_CANCEL:
    futureCancel(state, cmdBuf, fabric);
    break;
  case FUTURE_GET_PROGRESS:
    futureGetProgress(state, cmdBuf, fabric);
    break;
  case FINALIZE:
    finalize(state, cmdBuf, fabric);
    break;
  case NONE:
  default:
    throw std::runtime_error("Invalid work tag!");
    break;
  }
}

const char *tagName(work::TAG t)
{
  switch (t) {
  case NEW_RENDERER:
    return "NEW_RENDERER";
  case NEW_WORLD:
    return "NEW_WORLD";
  case NEW_GEOMETRY:
    return "NEW_GEOMETRY";
  case NEW_GEOMETRIC_MODEL:
    return "NEW_GEOMETRIC_MODEL";
  case NEW_VOLUME:
    return "NEW_VOLUME";
  case NEW_VOLUMETRIC_MODEL:
    return "NEW_VOLUMETRIC_MODEL";
  case NEW_CAMERA:
    return "NEW_CAMERA";
  case NEW_TRANSFER_FUNCTION:
    return "NEW_TRANSFER_FUNCTION";
  case NEW_IMAGE_OPERATION:
    return "NEW_IMAGE_OPERATION";
  case NEW_MATERIAL:
    return "NEW_MATERIAL";
  case NEW_LIGHT:
    return "NEW_LIGHT";
  case NEW_SHARED_DATA:
    return "NEW_SHARED_DATA";
  case NEW_DATA:
    return "NEW_DATA";
  case COPY_DATA:
    return "COPY_DATA";
  case NEW_TEXTURE:
    return "NEW_TEXTURE";
  case NEW_GROUP:
    return "NEW_GROUP";
  case NEW_INSTANCE:
    return "NEW_INSTANCE";
  case COMMIT:
    return "COMMIT";
  case RELEASE:
    return "RELEASE";
  case RETAIN:
    return "RETAIN";
  case LOAD_MODULE:
    return "LOAD_MODULE";
  case CREATE_FRAMEBUFFER:
    return "CREATE_FRAMEBUFFER";
  case MAP_FRAMEBUFFER:
    return "MAP_FRAMEBUFFER";
  case GET_VARIANCE:
    return "GET_VARIANCE";
  case RESET_ACCUMULATION:
    return "RESET_ACCUMULATION";
  case RENDER_FRAME_ASYNC:
    return "RENDER_FRAME_ASYNC";
  case SET_PARAM:
    return "SET_PARAM";
  case REMOVE_PARAM:
    return "REMOVE_PARAM";
  case SET_LOAD_BALANCER:
    return "SET_LOAD_BALANCER";
  case PICK:
    return "PICK";
  case FUTURE_IS_READY:
    return "FUTURE_IS_READY";
  case FUTURE_WAIT:
    return "FUTURE_WAIT";
  case FUTURE_CANCEL:
    return "FUTURE_CANCEL";
  case FUTURE_GET_PROGRESS:
    return "FUTURE_GET_PROGRESS";
  case FINALIZE:
    return "FINALIZE";
  case NONE:
  default:
    return "NONE/UNKNOWN/INVALID";
  }
}

} // namespace work
} // namespace mpi
} // namespace ospray
