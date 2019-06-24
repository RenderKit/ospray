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
#include "ospcommon/networking/BufferedDataStreaming.h"
// mpicommon
#include "mpiCommon/MPICommon.h"
// ospray
#include "api/Device.h"
#include "common/Managed.h"
// ospray::mpi
#include "common/OSPWork.h"

/*! \file MPIDevice.h Implements the "mpi" device for mpi rendering */

namespace ospray {
  namespace mpi {

    struct MPIOffloadDevice : public api::Device
    {
      MPIOffloadDevice();
      ~MPIOffloadDevice() override;

      /////////////////////////////////////////////////////////////////////////
      // ManagedObject Implementation /////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void commit() override;

      /////////////////////////////////////////////////////////////////////////
      // Device Implementation ////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      int loadModule(const char *name) override;

      // OSPRay Data Arrays ///////////////////////////////////////////////////

      OSPData newData(size_t nitems,
                      OSPDataType format,
                      const void *init,
                      int flags) override;

      int setRegion(OSPVolume object,
                    const void *source,
                    const vec3i &index,
                    const vec3i &count) override;

      // Renderable Objects ///////////////////////////////////////////////////

      OSPLight newLight(const char *type) override;
      OSPCamera newCamera(const char *type) override;
      OSPGeometry newGeometry(const char *type) override;
      OSPVolume newVolume(const char *type) override;

      OSPGeometricModel newGeometricModel(OSPGeometry geom) override;
      OSPVolumetricModel newVolumetricModel(OSPVolume volume) override;

      // Instancing ///////////////////////////////////////////////////////////

      OSPInstance newInstance() override;

      // Instance Meta-Data ///////////////////////////////////////////////////

      OSPMaterial newMaterial(const char *renderer_type,
                              const char *material_type) override;

      OSPTransferFunction newTransferFunction(const char *type) override;

      OSPTexture newTexture(const char *type) override;

      // World Manipulation ///////////////////////////////////////////////////

      OSPWorld newWorld() override;

      // Object Parameters ////////////////////////////////////////////////////

      void setString(OSPObject object,
                     const char *bufName,
                     const char *s) override;

      void setObject(OSPObject object,
                     const char *bufName,
                     OSPObject obj) override;

      void setBool(OSPObject object,
                   const char *bufName,
                   const bool f) override;

      void setFloat(OSPObject object,
                    const char *bufName,
                    const float f) override;

      void setInt(OSPObject object, const char *bufName, const int f) override;

      void setVec2f(OSPObject object,
                    const char *bufName,
                    const vec2f &v) override;

      void setVec2i(OSPObject object,
                    const char *bufName,
                    const vec2i &v) override;

      void setVec3f(OSPObject object,
                    const char *bufName,
                    const vec3f &v) override;

      void setVec3i(OSPObject object,
                    const char *bufName,
                    const vec3i &v) override;

      void setVec4f(OSPObject object,
                    const char *bufName,
                    const vec4f &v) override;

      void setVec4i(OSPObject object,
                    const char *bufName,
                    const vec4i &v) override;

      void setBox1f(OSPObject object,
                    const char *bufName,
                    const box1f &v) override;

      void setBox1i(OSPObject object,
                    const char *bufName,
                    const box1i &v) override;

      void setBox2f(OSPObject object,
                    const char *bufName,
                    const box2f &v) override;

      void setBox2i(OSPObject object,
                    const char *bufName,
                    const box2i &v) override;

      void setBox3f(OSPObject object,
                    const char *bufName,
                    const box3f &v) override;

      void setBox3i(OSPObject object,
                    const char *bufName,
                    const box3i &v) override;

      void setBox4f(OSPObject object,
                    const char *bufName,
                    const box4f &v) override;

      void setBox4i(OSPObject object,
                    const char *bufName,
                    const box4i &v) override;

      void setLinear3f(OSPObject object,
                       const char *bufName,
                       const linear3f &v) override;

      void setAffine3f(OSPObject object,
                       const char *bufName,
                       const affine3f &v) override;

      void setVoidPtr(OSPObject object, const char *bufName, void *v) override;

      // Object + Parameter Lifetime Management ///////////////////////////////

      void commit(OSPObject object) override;
      void removeParam(OSPObject object, const char *name) override;
      void release(OSPObject _obj) override;

      // FrameBuffer Manipulation /////////////////////////////////////////////

      OSPFrameBuffer frameBufferCreate(const vec2i &size,
                                       const OSPFrameBufferFormat mode,
                                       const uint32 channels) override;

      OSPPixelOp newPixelOp(const char *type) override;

      const void *frameBufferMap(OSPFrameBuffer fb,
                                 const OSPFrameBufferChannel) override;

      void frameBufferUnmap(const void *mapped, OSPFrameBuffer fb) override;

      float getVariance(OSPFrameBuffer) override;

      void resetAccumulation(OSPFrameBuffer _fb) override;

      // Frame Rendering //////////////////////////////////////////////////////

      OSPRenderer newRenderer(const char *type) override;

      float renderFrame(OSPFrameBuffer,
                        OSPRenderer,
                        OSPCamera,
                        OSPWorld) override;

      OSPFuture renderFrameAsync(OSPFrameBuffer,
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

     private:
      void initializeDevice();

      void processWork(work::Work &work, bool flushWriteStream = false);

      /*! This only exists to support getting the voxel type for setRegion */
      int getString(OSPObject object, const char *name, char **value);

      ObjectHandle allocateHandle() const;

      /*! @{ read and write stream for the work commands */
      std::unique_ptr<networking::Fabric> mpiFabric;
      std::unique_ptr<networking::ReadStream> readStream;
      std::unique_ptr<networking::WriteStream> writeStream;
      /*! @} */

      work::WorkTypeRegistry workRegistry;

      bool initialized{false};
    };

  }  // namespace mpi
}  // namespace ospray
