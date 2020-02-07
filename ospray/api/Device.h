// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospcommon
#include "ospcommon/memory/IntrusivePtr.h"
#include "ospcommon/utility/Optional.h"
#include "ospcommon/utility/ParameterizedObject.h"
// ospray
#include "../common/OSPCommon.h"
#include "ospray/version.h"
// std
#include <functional>

/*! \file device.h Defines the abstract base class for OSPRay
    "devices" that implement the OSPRay API */

namespace ospray {
namespace api {

/*! abstract base class of all 'devices' that implement the ospray API */
struct OSPRAY_CORE_INTERFACE Device : public memory::RefCountedObject,
                                      public utility::ParameterizedObject
{
  /*! singleton that points to currently active device */
  static memory::IntrusivePtr<Device> current;

  Device() = default;
  virtual ~Device() override = default;

  static Device *createDevice(const char *type);

  /*! gets a device property */
  virtual int64_t getProperty(const OSPDeviceProperty prop);

  /////////////////////////////////////////////////////////////////////////
  // Main virtual interface to accepting API calls
  /////////////////////////////////////////////////////////////////////////

  // Modules //////////////////////////////////////////////////////////////

  virtual int loadModule(const char *name) = 0;

  // OSPRay Data Arrays ///////////////////////////////////////////////////

  virtual OSPData newSharedData(const void *sharedData,
      OSPDataType,
      const vec3ul &numItems,
      const vec3l &byteStride) = 0;

  virtual OSPData newData(OSPDataType, const vec3ul &numItems) = 0;

  virtual void copyData(const OSPData source,
      OSPData destination,
      const vec3ul &destinationIndex) = 0;

  // Renderable Objects ///////////////////////////////////////////////////

  virtual OSPLight newLight(const char *type) = 0;

  virtual OSPCamera newCamera(const char *type) = 0;

  virtual OSPGeometry newGeometry(const char *type) = 0;
  virtual OSPVolume newVolume(const char *type) = 0;

  virtual OSPGeometricModel newGeometricModel(OSPGeometry geom) = 0;
  virtual OSPVolumetricModel newVolumetricModel(OSPVolume volume) = 0;

  // Model Meta-Data //////////////////////////////////////////////////////

  virtual OSPMaterial newMaterial(
      const char *renderer_type, const char *material_type) = 0;

  virtual OSPTransferFunction newTransferFunction(const char *type) = 0;

  virtual OSPTexture newTexture(const char *type) = 0;

  // Instancing ///////////////////////////////////////////////////////////

  virtual OSPGroup newGroup() = 0;
  virtual OSPInstance newInstance(OSPGroup group) = 0;

  // Top-level Worlds /////////////////////////////////////////////////////

  virtual OSPWorld newWorld() = 0;
  virtual box3f getBounds(OSPObject) = 0;

  // Object + Parameter Lifetime Management ///////////////////////////////

  virtual void setObjectParam(OSPObject object,
      const char *name,
      OSPDataType type,
      const void *mem) = 0;

  virtual void removeObjectParam(OSPObject object, const char *name) = 0;

  virtual void commit(OSPObject object) = 0;

  virtual void release(OSPObject _obj) = 0;
  virtual void retain(OSPObject _obj) = 0;

  // FrameBuffer Manipulation /////////////////////////////////////////////

  virtual OSPFrameBuffer frameBufferCreate(const vec2i &size,
      const OSPFrameBufferFormat mode,
      const uint32 channels) = 0;

  virtual OSPImageOperation newImageOp(const char *type) = 0;

  virtual const void *frameBufferMap(
      OSPFrameBuffer fb, const OSPFrameBufferChannel) = 0;

  virtual void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) = 0;

  virtual float getVariance(OSPFrameBuffer) = 0;

  virtual void resetAccumulation(OSPFrameBuffer _fb) = 0;

  // Frame Rendering //////////////////////////////////////////////////////

  virtual OSPRenderer newRenderer(const char *type) = 0;

  virtual OSPFuture renderFrame(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld) = 0;

  virtual int isReady(OSPFuture, OSPSyncEvent) = 0;
  virtual void wait(OSPFuture, OSPSyncEvent) = 0;
  virtual void cancel(OSPFuture) = 0;
  virtual float getProgress(OSPFuture) = 0;

  virtual OSPPickResult pick(
      OSPFrameBuffer, OSPRenderer, OSPCamera, OSPWorld, const vec2f &)
  {
    NOT_IMPLEMENTED;
  }

  /////////////////////////////////////////////////////////////////////////
  // Helper/other functions and data members
  /////////////////////////////////////////////////////////////////////////

  virtual void commit();
  bool isCommitted();

  static utility::Optional<int> logLevelFromString(const std::string &str);

  int numThreads{-1};
  bool debugMode{false};
  bool apiTraceEnabled{false};

  enum OSP_THREAD_AFFINITY
  {
    DEAFFINITIZE = 0,
    AFFINITIZE = 1,
    AUTO_DETECT = 2
  };

  int threadAffinity{AUTO_DETECT};

  // NOTE(jda) - Keep logLevel static because the device factory function
  //             needs to have a valid value for the initial Device creation
  static uint32_t logLevel;

  bool warningsAreErrors{false};

  std::function<void(const char *)> msg_fcn{[](const char *) {}};

  std::function<void(OSPError, const char *)> error_fcn{
      [](OSPError, const char *) {}};

  std::function<void(const char *)> trace_fcn{[](const char *) {}};

  OSPError lastErrorCode = OSP_NO_ERROR;
  std::string lastErrorMsg = "no error"; // no braced initializer for MSVC12

 private:
  bool committed{false};
};

// Shorthand functions to query current API device //

OSPRAY_CORE_INTERFACE bool deviceIsSet();
OSPRAY_CORE_INTERFACE Device &currentDevice();

OSPRAY_CORE_INTERFACE
std::string generateEmbreeDeviceCfg(const Device &device);

/*! \brief registers a internal ospray::<ClassName> renderer under
    the externally accessible name "external_name"

    \internal This currently works by defining a extern "C" function
    with a given predefined name that creates a new instance of this
    renderer. By having this symbol in the shared lib ospray can
    lateron always get a handle to this fct and create an instance
    of this renderer.
*/
#define OSP_REGISTER_DEVICE(InternalClass, external_name)                      \
  OSP_REGISTER_OBJECT(                                                         \
      ::ospray::api::Device, device, InternalClass, external_name)

} // namespace api

OSPTYPEFOR_SPECIALIZATION(api::Device *, OSP_DEVICE);

} // namespace ospray
