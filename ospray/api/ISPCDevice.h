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

// ospray
#include "Device.h"
// embree
#include "embree3/rtcore.h"

/*! \file ISPCDevice.h Implements the "local" device for local rendering */

extern "C" OSPRAY_SDK_INTERFACE RTCDevice ispc_embreeDevice();

namespace ospray {
  namespace api {

    struct OSPRAY_SDK_INTERFACE ISPCDevice : public Device
    {
      ISPCDevice() = default;
      ~ISPCDevice() override;

      /////////////////////////////////////////////////////////////////////////
      // ManagedObject Implementation /////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void commit() override;

      /////////////////////////////////////////////////////////////////////////
      // Device Implementation ////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      int loadModule(const char *name) override;

      // OSPRay Data Arrays ///////////////////////////////////////////////////

      OSPData newSharedData(const void *sharedData,
                            OSPDataType,
                            const vec3i &numItems,
                            const vec3l &byteStride) override;

      OSPData newData(OSPDataType, const vec3i &numItems) override;

      void copyData(const OSPData source,
                    OSPData destination,
                    const vec3i &DestinationIndex) override;

      // Renderable Objects ///////////////////////////////////////////////////

      OSPLight newLight(const char *type) override;

      OSPCamera newCamera(const char *type) override;

      OSPGeometry newGeometry(const char *type) override;
      OSPVolume newVolume(const char *type) override;

      OSPGeometricModel newGeometricModel(OSPGeometry geom) override;
      OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

      // Model Meta-Data //////////////////////////////////////////////////////

      OSPMaterial newMaterial(const char *renderer_type,
                              const char *material_type) override;

      OSPTransferFunction newTransferFunction(const char *type) override;

      OSPTexture newTexture(const char *type) override;

      // Instancing ///////////////////////////////////////////////////////////

      OSPGroup newGroup() override;
      OSPInstance newInstance(OSPGroup group) override;

      // Top-level Worlds /////////////////////////////////////////////////////

      OSPWorld newWorld() override;
      box3f getBounds(OSPObject) override;

      // Object + Parameter Lifetime Management ///////////////////////////////

      void setObjectParam(OSPObject object,
                          const char *name,
                          OSPDataType type,
                          const void *mem) override;

      void removeObjectParam(OSPObject object, const char *name) override;

      void commit(OSPObject object) override;

      void release(OSPObject _obj) override;
      void retain(OSPObject _obj) override;

      // FrameBuffer Manipulation /////////////////////////////////////////////

      OSPFrameBuffer frameBufferCreate(const vec2i &size,
                                       const OSPFrameBufferFormat mode,
                                       const uint32 channels) override;

      OSPImageOperation newImageOp(const char *type) override;

      const void *frameBufferMap(OSPFrameBuffer fb,
                                 const OSPFrameBufferChannel) override;

      void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;

      float getVariance(OSPFrameBuffer) override;

      void resetAccumulation(OSPFrameBuffer _fb) override;

      // Frame Rendering //////////////////////////////////////////////////////

      OSPRenderer newRenderer(const char *type) override;

      OSPFuture renderFrame(OSPFrameBuffer,
                            OSPRenderer,
                            OSPCamera,
                            OSPWorld) override;

      int isReady(OSPFuture, OSPSyncEvent) override;
      void wait(OSPFuture, OSPSyncEvent) override;
      void cancel(OSPFuture) override;
      float getProgress(OSPFuture) override;

      OSPPickResult pick(OSPFrameBuffer,
                         OSPRenderer,
                         OSPCamera,
                         OSPWorld,
                         const vec2f &) override;

      // Public Data //

      // NOTE(jda) - Keep embreeDevice static until runWorker() in MPI mode can
      //             safely assume that a device exists.
      static RTCDevice embreeDevice;
    };

  }  // namespace api
}  // namespace ospray
