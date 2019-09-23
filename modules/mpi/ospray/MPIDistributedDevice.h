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

#include "api/Device.h"
#include "common/MPICommon.h"
#include "common/Managed.h"
#include "common/ObjectHandle.h"

namespace ospray {
namespace mpi {

template <typename OSPRAY_TYPE>
inline OSPRAY_TYPE &lookupDistributedObject(OSPObject obj)
{
  auto &handle = reinterpret_cast<ObjectHandle &>(obj);
  auto *object = (OSPRAY_TYPE *)handle.lookup();

  if (!object)
    throw std::runtime_error("#dmpi: ObjectHandle doesn't exist!");

  return *object;
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
  OSPImageOp newImageOp(const char *type) override;

  void resetAccumulation(OSPFrameBuffer _fb) override;

  // Instancing ///////////////////////////////////////////////////////////

  OSPGroup newGroup() override;
  OSPInstance newInstance(OSPGroup group) override;

  // Top-level Worlds /////////////////////////////////////////////////////

  OSPWorld newWorld() override;

  /*! commit the given object's outstanding changes */
  void commit(OSPObject object) override;

  // OSPRay Data Arrays ///////////////////////////////////////////////////

  OSPData newSharedData(const void *sharedData,
      OSPDataType,
      const vec3i &numItems,
      const vec3l &byteStride) override;

  OSPData newData(OSPDataType, const vec3i &numItems) override;

  void copyData(const OSPData source,
      OSPData destination,
      const vec3i &DestinationIndex) override;

  /*! assign (named) string parameter to an object */
  void setString(OSPObject object, const char *bufName, const char *s) override;

  /*! assign (named) data item as a parameter to an object */
  void setObject(
      OSPObject target, const char *bufName, OSPObject value) override;

  /*! assign (named) float parameter to an object */
  void setBool(OSPObject object, const char *bufName, const bool b) override;

  /*! assign (named) float parameter to an object */
  void setFloat(OSPObject object, const char *bufName, const float f) override;

  /*! assign (named) vec2f parameter to an object */
  void setVec2f(OSPObject object, const char *bufName, const vec2f &v) override;

  /*! assign (named) vec3f parameter to an object */
  void setVec3f(OSPObject object, const char *bufName, const vec3f &v) override;

  /*! assign (named) vec4f parameter to an object */
  void setVec4f(OSPObject object, const char *bufName, const vec4f &v) override;

  /*! assign (named) int parameter to an object */
  void setInt(OSPObject object, const char *bufName, const int f) override;

  /*! assign (named) vec2i parameter to an object */
  void setVec2i(OSPObject object, const char *bufName, const vec2i &v) override;

  /*! assign (named) vec3i parameter to an object */
  void setVec3i(OSPObject object, const char *bufName, const vec3i &v) override;

  void setVec4i(OSPObject object, const char *bufName, const vec4i &v) override;

  void setBox1f(OSPObject object, const char *bufName, const box1f &v) override;

  void setBox1i(OSPObject object, const char *bufName, const box1i &v) override;

  void setBox2f(OSPObject object, const char *bufName, const box2f &v) override;

  void setBox2i(OSPObject object, const char *bufName, const box2i &v) override;

  void setBox3f(OSPObject object, const char *bufName, const box3f &v) override;

  void setBox3i(OSPObject object, const char *bufName, const box3i &v) override;

  void setBox4f(OSPObject object, const char *bufName, const box4f &v) override;

  void setBox4i(OSPObject object, const char *bufName, const box4i &v) override;

  void setLinear3f(
      OSPObject object, const char *bufName, const linear3f &v) override;

  void setAffine3f(
      OSPObject object, const char *bufName, const affine3f &v) override;

  /*! add untyped void pointer to object - this will *ONLY* work in local
      rendering!  */
  void setVoidPtr(OSPObject object, const char *bufName, void *v) override;

  void removeParam(OSPObject object, const char *name) override;

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

  /*! call a renderer to render a frame buffer */
  float renderFrame(OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) override;

  OSPFuture renderFrameAsync(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) override;

  int isReady(OSPFuture, OSPSyncEvent) override;

  void wait(OSPFuture, OSPSyncEvent) override;

  void cancel(OSPFuture) override;

  float getProgress(OSPFuture) override;

  float getVariance(OSPFrameBuffer) override;

  /*! load module */
  int loadModule(const char *name) override;

  //! release (i.e., reduce refcount of) given object
  /*! note that all objects in ospray are refcounted, so one cannot
    explicitly "delete" any object. instead, each object is created
    with a refcount of 1, and this refcount will be
    increased/decreased every time another object refers to this
    object resp releases its hold on it override; if the refcount is 0 the
    object will automatically get deleted. For example, you can
    create a new material, assign it to a geometry, and immediately
    after this assignation release its refcount override; the material will
    stay 'alive' as long as the given geometry requires it. */
  void release(OSPObject _obj) override;

  /*! create a new Texture object */
  OSPTexture newTexture(const char *type) override;

 private:
  bool initialized{false};
  bool shouldFinalizeMPI{false};
};

} // namespace mpi
} // namespace ospray
