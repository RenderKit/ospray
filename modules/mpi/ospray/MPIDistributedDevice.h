// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
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

#include "api/Device.h"
#include "common/MPICommon.h"
#include "common/Managed.h"
#include "common/ObjectHandle.h"

namespace ospray {
namespace mpi {

template <typename OSPRAY_TYPE>
inline OSPRAY_TYPE *lookupDistributedObject(OSPObject obj)
{
  auto &handle = reinterpret_cast<ObjectHandle &>(obj);
  auto *object = (OSPRAY_TYPE *)handle.lookup();

  if (!object)
    throw std::runtime_error("#dmpi: ObjectHandle doesn't exist!");

  return object;
}

template <typename OSPRAY_TYPE>
inline OSPRAY_TYPE *lookupObject(OSPObject obj)
{
  auto &handle = reinterpret_cast<ObjectHandle &>(obj);
  if (handle.defined()) {
    return (OSPRAY_TYPE *)handle.lookup();
  } else {
    return (OSPRAY_TYPE *)obj;
  }
}

struct MPIDistributedDevice : public api::Device
{
  MPIDistributedDevice();
  ~MPIDistributedDevice() override;

  // ManagedObject Implementation /////////////////////////////////////////

  void commit() override;

  // Device Implementation ////////////////////////////////////////////////

  /*! create a new frame buffer */
  OSPFrameBuffer frameBufferCreate(const vec2i &size,
      const OSPFrameBufferFormat mode,
      const uint32 channels) override;

  /*! create a new transfer function object (out of list of
    registered transfer function types) */
  OSPTransferFunction newTransferFunction(const char *type) override;

  /*! have given renderer create a new Light */
  OSPLight newLight(const char *light_type) override;

  /*! map frame buffer */
  const void *frameBufferMap(
      OSPFrameBuffer fb, OSPFrameBufferChannel channel) override;

  /*! unmap previously mapped frame buffer */
  void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;

  /*! create a new pixelOp object (out of list of registered pixelOps) */
  OSPImageOperation newImageOp(const char *type) override;

  void resetAccumulation(OSPFrameBuffer _fb) override;

  // Instancing ///////////////////////////////////////////////////////////

  OSPGroup newGroup() override;
  OSPInstance newInstance(OSPGroup group) override;

  // Top-level Worlds /////////////////////////////////////////////////////

  OSPWorld newWorld() override;
  box3f getBounds(OSPObject) override;

  // OSPRay Data Arrays ///////////////////////////////////////////////////

  OSPData newSharedData(const void *sharedData,
      OSPDataType,
      const vec3ul &numItems,
      const vec3l &byteStride) override;

  OSPData newData(OSPDataType, const vec3ul &numItems) override;

  void copyData(const OSPData source,
      OSPData destination,
      const vec3ul &destinationIndex) override;

  /*! create a new renderer object (out of list of registered renderers) */
  OSPRenderer newRenderer(const char *type) override;

  /*! create a new geometry object (out of list of registered geometries) */
  OSPGeometry newGeometry(const char *type) override;

  /*! have given renderer create a new material */
  OSPMaterial newMaterial(
      const char *renderer_type, const char *material_type) override;

  /*! create a new camera object (out of list of registered cameras) */
  OSPCamera newCamera(const char *type) override;

  /*! create a new volume object (out of list of registered volumes) */
  OSPVolume newVolume(const char *type) override;

  OSPGeometricModel newGeometricModel(OSPGeometry geom) override;

  OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

  OSPFuture renderFrame(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) override;

  int isReady(OSPFuture, OSPSyncEvent) override;

  void wait(OSPFuture, OSPSyncEvent) override;

  void cancel(OSPFuture) override;

  float getProgress(OSPFuture) override;

  float getTaskDuration(OSPFuture) override;

  float getVariance(OSPFrameBuffer) override;

  /*! load module */
  int loadModule(const char *name) override;

  // Object + Parameter Lifetime Management ///////////////////////////////

  void setObjectParam(OSPObject object,
      const char *name,
      OSPDataType type,
      const void *mem) override;

  void removeObjectParam(OSPObject object, const char *name) override;

  void commit(OSPObject object) override;
  void release(OSPObject _obj) override;
  void retain(OSPObject _obj) override;

  /*! create a new Texture object */
  OSPTexture newTexture(const char *type) override;

 private:
  bool initialized{false};
  bool shouldFinalizeMPI{false};
};

} // namespace mpi
} // namespace ospray
