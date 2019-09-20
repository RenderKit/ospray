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

// ospcommon
#include "ospcommon/utility/ParameterizedObject.h"
// ospray
#include "../common/OSPCommon.h"
// std
#include <functional>

/*! \file device.h Defines the abstract base class for OSPRay
    "devices" that implement the OSPRay API */

namespace ospray {
  namespace api {

    /*! abstract base class of all 'devices' that implement the ospray API */
    struct OSPRAY_CORE_INTERFACE Device : public utility::ParameterizedObject
    {
      /*! singleton that points to currently active device */
      static std::shared_ptr<Device> current;

      Device()                   = default;
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

      virtual OSPData newData(size_t nitems,
                              OSPDataType format,
                              const void *init,
                              int flags) = 0;

      // Renderable Objects ///////////////////////////////////////////////////

      virtual OSPLight newLight(const char *type) = 0;

      virtual OSPCamera newCamera(const char *type) = 0;

      virtual OSPGeometry newGeometry(const char *type) = 0;
      virtual OSPVolume newVolume(const char *type)     = 0;

      virtual OSPGeometricModel newGeometricModel(OSPGeometry geom)   = 0;
      virtual OSPVolumetricModel newVolumetricModel(OSPVolume volume) = 0;

      // Model Meta-Data //////////////////////////////////////////////////////

      virtual OSPMaterial newMaterial(const char *renderer_type,
                                      const char *material_type) = 0;

      virtual OSPTransferFunction newTransferFunction(const char *type) = 0;

      virtual OSPTexture newTexture(const char *type) = 0;

      // Instancing ///////////////////////////////////////////////////////////

      virtual OSPGroup newGroup() = 0;
      virtual OSPInstance newInstance(OSPGroup group) = 0;

      // Top-level Worlds /////////////////////////////////////////////////////

      virtual OSPWorld newWorld() = 0;

      // Object Parameters ////////////////////////////////////////////////////

      virtual void setString(OSPObject object,
                             const char *bufName,
                             const char *s) = 0;

      virtual void setObject(OSPObject object,
                             const char *bufName,
                             OSPObject obj) = 0;

      virtual void setBool(OSPObject object,
                           const char *bufName,
                           const bool f) = 0;

      virtual void setFloat(OSPObject object,
                            const char *bufName,
                            const float f) = 0;

      virtual void setInt(OSPObject object,
                          const char *bufName,
                          const int f) = 0;

      virtual void setVec2f(OSPObject object,
                            const char *bufName,
                            const vec2f &v) = 0;

      virtual void setVec2i(OSPObject object,
                            const char *bufName,
                            const vec2i &v) = 0;

      virtual void setVec3f(OSPObject object,
                            const char *bufName,
                            const vec3f &v) = 0;

      virtual void setVec3i(OSPObject object,
                            const char *bufName,
                            const vec3i &v) = 0;

      virtual void setVec4f(OSPObject object,
                            const char *bufName,
                            const vec4f &v) = 0;

      virtual void setVec4i(OSPObject object,
                            const char *bufName,
                            const vec4i &v) = 0;

      virtual void setBox1f(OSPObject object,
                            const char *bufName,
                            const box1f &v) = 0;

      virtual void setBox1i(OSPObject object,
                            const char *bufName,
                            const box1i &v) = 0;

      virtual void setBox2f(OSPObject object,
                            const char *bufName,
                            const box2f &v) = 0;

      virtual void setBox2i(OSPObject object,
                            const char *bufName,
                            const box2i &v) = 0;

      virtual void setBox3f(OSPObject object,
                            const char *bufName,
                            const box3f &v) = 0;

      virtual void setBox3i(OSPObject object,
                            const char *bufName,
                            const box3i &v) = 0;

      virtual void setBox4f(OSPObject object,
                            const char *bufName,
                            const box4f &v) = 0;

      virtual void setBox4i(OSPObject object,
                            const char *bufName,
                            const box4i &v) = 0;

      virtual void setLinear3f(OSPObject object,
                               const char *bufName,
                               const linear3f &v) = 0;

      virtual void setAffine3f(OSPObject object,
                               const char *bufName,
                               const affine3f &v) = 0;

      virtual void setVoidPtr(OSPObject object,
                              const char *bufName,
                              void *v) = 0;

      // Object + Parameter Lifetime Management ///////////////////////////////

      virtual void commit(OSPObject object)                        = 0;
      virtual void removeParam(OSPObject object, const char *name) = 0;
      virtual void release(OSPObject _obj)                         = 0;

      // FrameBuffer Manipulation /////////////////////////////////////////////

      virtual OSPFrameBuffer frameBufferCreate(const vec2i &size,
                                               const OSPFrameBufferFormat mode,
                                               const uint32 channels) = 0;

      virtual OSPImageOp newImageOp(const char *type) = 0;

      virtual const void *frameBufferMap(OSPFrameBuffer fb,
                                         const OSPFrameBufferChannel) = 0;

      virtual void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) = 0;

      virtual float getVariance(OSPFrameBuffer) = 0;

      virtual void resetAccumulation(OSPFrameBuffer _fb) = 0;

      // Frame Rendering //////////////////////////////////////////////////////

      virtual OSPRenderer newRenderer(const char *type) = 0;

      virtual float renderFrame(OSPFrameBuffer,
                                OSPRenderer,
                                OSPCamera,
                                OSPWorld) = 0;

      virtual OSPFuture renderFrameAsync(OSPFrameBuffer,
                                         OSPRenderer,
                                         OSPCamera,
                                         OSPWorld) = 0;

      virtual int isReady(OSPFuture, OSPSyncEvent) = 0;
      virtual void wait(OSPFuture, OSPSyncEvent)   = 0;
      virtual void cancel(OSPFuture)               = 0;
      virtual float getProgress(OSPFuture)         = 0;

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

      int numThreads{-1};
      /*! whether we're running in debug mode (cmdline: --osp:debug) */
      bool debugMode{false};
      bool apiTraceEnabled{false};

      enum OSP_THREAD_AFFINITY
      {
        DEAFFINITIZE = 0,
        AFFINITIZE   = 1,
        AUTO_DETECT  = 2
      };

      int threadAffinity{AUTO_DETECT};

      /*! logging level (cmdline: --osp:loglevel \<n\>) */
      // NOTE(jda) - Keep logLevel static because the device factory function
      //             needs to have a valid value for the initial Device creation
      static uint32_t logLevel;

      std::function<void(const char *)> msg_fcn{[](const char *) {}};

      std::function<void(OSPError, const char *)> error_fcn{
          [](OSPError, const char *) {}};

      std::function<void(const char *)> trace_fcn{[](const char *) {}};

      OSPError lastErrorCode = OSP_NO_ERROR;
      std::string lastErrorMsg =
          "no error";  // no braced initializer for MSVC12


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
#define OSP_REGISTER_DEVICE(InternalClass, external_name) \
  OSP_REGISTER_OBJECT(                                    \
      ::ospray::api::Device, device, InternalClass, external_name)

  }  // namespace api
}  // namespace ospray
