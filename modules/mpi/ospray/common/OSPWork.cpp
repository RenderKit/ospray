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

#include "ospcommon/utility/ArrayView.h"
#include "ospcommon/utility/OwnedArray.h"
#include "OSPWork.h"
#include "../fb/DistributedFrameBuffer.h"
#include "MPICommon.h"
#include "ospray/common/ObjectHandle.h"
#include "render/RenderTask.h"

#include "common/Data.h"
#include "common/Library.h"
#include "common/World.h"
#include "geometry/GeometricModel.h"
#include "texture/Texture.h"
#include "volume/VolumetricModel.h"

namespace ospray {
  namespace mpi {
    namespace work {

      FrameBufferInfo::FrameBufferInfo(const vec2i &size,
                                       OSPFrameBufferFormat format,
                                       uint32_t channels)
        : size(size),
        format(format),
        channels(channels)
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

      void newRenderer(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewRenderer(type.c_str());
      }

      void newWorld(OSPState &state,
                    networking::BufferReader &cmdBuf,
                    networking::Fabric &)
      {
        int64_t handle = 0;
        cmdBuf >> handle;
        state.objects[handle] = ospNewWorld();
      }

      void newGeometry(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewGeometry(type.c_str());
      }

      void newGeometricModel(OSPState &state,
                             networking::BufferReader &cmdBuf,
                             networking::Fabric &)
      {
        int64_t handle = 0;
        int64_t geomHandle = 0;
        cmdBuf >> handle >> geomHandle;
        state.objects[handle] =
          ospNewGeometricModel(state.getObject<OSPGeometry>(geomHandle));
      }

      void newVolume(OSPState &state,
                     networking::BufferReader &cmdBuf,
                     networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewVolume(type.c_str());
      }

      void newVolumetricModel(OSPState &state,
                              networking::BufferReader &cmdBuf,
                              networking::Fabric &)
      {
        int64_t handle = 0;
        int64_t volHandle = 0;
        cmdBuf >> handle >> volHandle;
        state.objects[handle] =
          ospNewVolumetricModel(state.getObject<OSPVolume>(volHandle));
      }

      void newCamera(OSPState &state,
                     networking::BufferReader &cmdBuf,
                     networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewCamera(type.c_str());
      }

      void newTransferFunction(OSPState &state,
                               networking::BufferReader &cmdBuf,
                               networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewTransferFunction(type.c_str());
      }

      void newImageOp(OSPState &state,
                      networking::BufferReader &cmdBuf,
                      networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewImageOp(type.c_str());
      }

      void newMaterial(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type, rendererType;
        cmdBuf >> handle >> rendererType >> type;
        state.objects[handle] = ospNewMaterial(rendererType.c_str(), type.c_str());
      }

      void newLight(OSPState &state,
                    networking::BufferReader &cmdBuf,
                    networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewLight(type.c_str());
      }

      void newData(OSPState &state,
                   networking::BufferReader &cmdBuf,
                   networking::Fabric &fabric)
      {
        using namespace utility;

        int64_t handle = 0;
        size_t nitems = 0;
        OSPDataType format;
        int flags = 0;
        cmdBuf >> handle >> nitems >> format >> flags;

        auto view = ospcommon::make_unique<OwnedArray<uint8_t>>();
        view->resize(sizeOf(format) * nitems, 0);
        fabric.recvBcast(*view);

        // TODO if the data type is managed we need to convert the handles
        // back into pointers
        if (isManagedObject(format))
        {
          int64_t *handles = reinterpret_cast<int64_t*>(view->data());
          OSPObject *objs = reinterpret_cast<OSPObject*>(view->data());
          for (size_t i = 0; i < nitems; ++i)
            objs[i] = state.objects[handles[i]];
        }

        flags = flags & !OSP_DATA_SHARED_BUFFER;

        state.objects[handle] = ospNewData(nitems, format, view->data(), flags);
      }

      void newTexture(OSPState &state,
                      networking::BufferReader &cmdBuf,
                      networking::Fabric &)
      {
        int64_t handle = 0;
        std::string type;
        cmdBuf >> handle >> type;
        state.objects[handle] = ospNewTexture(type.c_str());
      }

      void newGroup(OSPState &state,
                    networking::BufferReader &cmdBuf,
                    networking::Fabric &)
      {
        int64_t handle = 0;
        cmdBuf >> handle;
        state.objects[handle] = ospNewGroup();
      }

      void newInstance(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        int64_t groupHandle = 0;
        cmdBuf >> handle >> groupHandle;
        state.objects[handle] =
          ospNewInstance(state.getObject<OSPGroup>(groupHandle));
      }

      void commit(OSPState &state,
                  networking::BufferReader &cmdBuf,
                  networking::Fabric &)
      {
        int64_t handle = 0;
        cmdBuf >> handle;
        ospCommit(state.objects[handle]);
      }

      void release(OSPState &state,
                   networking::BufferReader &cmdBuf,
                   networking::Fabric &)
      {
        int64_t handle = 0;
        cmdBuf >> handle;

        ospRelease(state.objects[handle]);

        auto f = state.framebuffers.find(handle);
        if (f != state.framebuffers.end())
        {
          state.framebuffers.erase(f);
        }
      }

      void loadModule(OSPState &state,
                      networking::BufferReader &cmdBuf,
                      networking::Fabric &)
      {
        std::string module;
        cmdBuf >> module;
        ospLoadModule(module.c_str());
      }

      void createFramebuffer(OSPState &state,
                             networking::BufferReader &cmdBuf,
                             networking::Fabric &)
      {
        int64_t handle = 0;
        vec2i size(0, 0);
        uint32_t format;
        uint32_t channels = 0;
        cmdBuf >> handle >> size >> format >> channels;
        state.objects[handle] = ospNewFrameBuffer(size.x, size.y,
                                                  (OSPFrameBufferFormat)format,
                                                  channels);
        state.framebuffers[handle] =
          FrameBufferInfo(size, (OSPFrameBufferFormat)format, channels);
      }

      void mapFramebuffer(OSPState &state,
                          networking::BufferReader &cmdBuf,
                          networking::Fabric &fabric)
      {
        // Map the channel and send the image back over the fabric
        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;

          int64_t handle = 0;
          uint32_t channel = 0;
          cmdBuf >> handle >> channel;

          const FrameBufferInfo &fbInfo = state.framebuffers[handle];
          uint64_t nbytes = fbInfo.pixelSize(channel) * fbInfo.getNumPixels();

          auto bytesView =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&nbytes),
                                                       sizeof(nbytes));
          fabric.send(bytesView, 0);

          if (nbytes != 0)
          {
            OSPFrameBuffer fb = state.getObject<OSPFrameBuffer>(handle);
            void *map = const_cast<void*>(ospMapFrameBuffer(fb, (OSPFrameBufferChannel)channel));

            auto fbView = 
              std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(map),
                                                         nbytes);

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
        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;

          int64_t handle = 0;
          cmdBuf >> handle;

          float variance =
            ospGetVariance(state.getObject<OSPFrameBuffer>(handle));

          auto bytesView =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&variance),
                                                       sizeof(variance));
          fabric.send(bytesView, 0);
        }
      }


      void resetAccumulation(OSPState &state,
                             networking::BufferReader &cmdBuf,
                             networking::Fabric &)
      {
        int64_t handle = 0;
        cmdBuf >> handle;
        ospResetAccumulation(state.getObject<OSPFrameBuffer>(handle));
      }

      void renderFrameAsync(OSPState &state,
                            networking::BufferReader &cmdBuf,
                            networking::Fabric &)
      {
        int64_t futureHandle   = 0;
        int64_t fbHandle       = 0;
        int64_t rendererHandle = 0;
        int64_t cameraHandle   = 0;
        int64_t worldHandle    = 0;
        cmdBuf >> fbHandle >> rendererHandle >> cameraHandle >> worldHandle
               >> futureHandle;
        state.objects[futureHandle]
          = ospRenderFrameAsync(state.getObject<OSPFrameBuffer>(fbHandle),
                                state.getObject<OSPRenderer>(rendererHandle),
                                state.getObject<OSPCamera>(cameraHandle),
                                state.getObject<OSPWorld>(worldHandle));
      }

      void setParamObject(OSPState &state,
                          networking::BufferReader &cmdBuf,
                          networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        int64_t val = 0;
        cmdBuf >> handle >> param >> val;
        ospSetObject(state.objects[handle], param.c_str(), state.objects[val]);
      }

      void setParamString(OSPState &state,
                          networking::BufferReader &cmdBuf,
                          networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        std::string val;
        cmdBuf >> handle >> param >> val;
        ospSetString(state.objects[handle], param.c_str(), val.c_str());
      }

      void setParamInt(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        int32_t val;
        cmdBuf >> handle >> param >> val;
        ospSetInt(state.objects[handle], param.c_str(), val);
      }

      void setParamBool(OSPState &state,
                        networking::BufferReader &cmdBuf,
                        networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        bool val;
        cmdBuf >> handle >> param >> val;
        ospSetBool(state.objects[handle], param.c_str(), val);
      }

      void setParamFloat(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        float val;
        cmdBuf >> handle >> param >> val;
        ospSetFloat(state.objects[handle], param.c_str(), val);
      }

      void setParamVec2f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec2f val;
        cmdBuf >> handle >> param >> val;
        ospSetVec2fv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamVec2i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec2i val;
        cmdBuf >> handle >> param >> val;
        ospSetVec2iv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamVec3f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec3f val;
        cmdBuf >> handle >> param >> val;
        ospSetVec3fv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamVec3i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec3i val;
        cmdBuf >> handle >> param >> val;
        ospSetVec3iv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamVec4f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec4f val;
        cmdBuf >> handle >> param >> val;
        ospSetVec4fv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamVec4i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        vec4i val;
        cmdBuf >> handle >> param >> val;
        ospSetVec4iv(state.objects[handle], param.c_str(), &val.x);
      }

      void setParamBox1f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box1f val;
        cmdBuf >> handle >> param >> val;
        ospSetBox1fv(state.objects[handle], param.c_str(), &val.lower);
      }

      void setParamBox1i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box1i val;
        cmdBuf >> handle >> param >> val;
        ospSetBox1iv(state.objects[handle], param.c_str(), &val.lower);
      }

      void setParamBox2f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box2f val;
        cmdBuf >> handle >> param >> val;
        ospSetBox2fv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamBox2i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box2i val;
        cmdBuf >> handle >> param >> val;
        ospSetBox2iv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamBox3f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box3f val;
        cmdBuf >> handle >> param >> val;
        ospSetBox3fv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamBox3i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box3i val;
        cmdBuf >> handle >> param >> val;
        ospSetBox3iv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamBox4f(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box4f val;
        cmdBuf >> handle >> param >> val;
        ospSetBox4fv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamBox4i(OSPState &state,
                         networking::BufferReader &cmdBuf,
                         networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        box4i val;
        cmdBuf >> handle >> param >> val;
        ospSetBox4iv(state.objects[handle], param.c_str(), &val.lower.x);
      }

      void setParamLinear3f(OSPState &state,
                            networking::BufferReader &cmdBuf,
                            networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        linear3f val;
        cmdBuf >> handle >> param >> val;
        ospSetLinear3fv(state.objects[handle], param.c_str(), &val.vx.x);
      }

      void setParamAffine3f(OSPState &state,
                            networking::BufferReader &cmdBuf,
                            networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        affine3f val;
        cmdBuf >> handle >> param >> val;
        ospSetAffine3fv(state.objects[handle], param.c_str(),
                        reinterpret_cast<float*>(&val));
      }

      void removeParam(OSPState &state,
                       networking::BufferReader &cmdBuf,
                       networking::Fabric &)
      {
        int64_t handle = 0;
        std::string param;
        cmdBuf >> handle >> param;
        ospRemoveParam(state.objects[handle], param.c_str());
      }

      void setLoadBalancer(OSPState &state,
                           networking::BufferReader &cmdBuf,
                           networking::Fabric &)
      {
        PING;
        NOT_IMPLEMENTED;
        // TODO: We only have one load balancer now
      }

      void pick(OSPState &state,
                networking::BufferReader &cmdBuf,
                networking::Fabric &fabric)
      {
        int64_t fbHandle       = 0;
        int64_t rendererHandle = 0;
        int64_t cameraHandle   = 0;
        int64_t worldHandle    = 0;
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

        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;
          auto view =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&res),
                                                       sizeof(OSPPickResult));
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
        int ready = ospIsReady(state.getObject<OSPFuture>(handle),
                               (OSPSyncEvent)event);

        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;
          auto view =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&ready),
                                                       sizeof(ready));
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

        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;
          auto view =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&event),
                                                       sizeof(event));
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

        if (mpicommon::worker.rank == 0)
        {
          using namespace utility;
          auto view =
            std::make_shared<OwnedArray<uint8_t>>(reinterpret_cast<uint8_t*>(&progress),
                                                       sizeof(progress));
          fabric.send(view, 0);
        }
      }

      void finalize(OSPState &state,
                    networking::BufferReader &cmdBuf,
                    networking::Fabric &)
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
          case NEW_IMAGE_OP:
            newImageOp(state, cmdBuf, fabric);
            break;
          case NEW_MATERIAL:
            newMaterial(state, cmdBuf, fabric);
            break;
          case NEW_LIGHT:
            newLight(state, cmdBuf, fabric);
            break;
          case NEW_DATA:
            newData(state, cmdBuf, fabric);
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
          case SET_PARAM_OBJECT:
            setParamObject(state, cmdBuf, fabric);
            break;
          case SET_PARAM_STRING:
            setParamString(state, cmdBuf, fabric);
            break;
          case SET_PARAM_INT:
            setParamInt(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOOL:
            setParamBool(state, cmdBuf, fabric);
            break;
          case SET_PARAM_FLOAT:
            setParamFloat(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC2F:
            setParamVec2f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC2I:
            setParamVec2i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC3F:
            setParamVec3f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC3I:
            setParamVec3i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC4F:
            setParamVec4f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_VEC4I:
            setParamVec4i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX1F:
            setParamBox1f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX1I:
            setParamBox1i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX2F:
            setParamBox2f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX2I:
            setParamBox2i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX3F:
            setParamBox3f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX3I:
            setParamBox3i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX4F:
            setParamBox4f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_BOX4I:
            setParamBox4i(state, cmdBuf, fabric);
            break;
          case SET_PARAM_LINEAR3F:
            setParamLinear3f(state, cmdBuf, fabric);
            break;
          case SET_PARAM_AFFINE3F:
            setParamAffine3f(state, cmdBuf, fabric);
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

      const char* tagName(work::TAG t)
      {
        switch (t)
        {
          case NEW_RENDERER: return "NEW_RENDERER";
          case NEW_WORLD: return "NEW_WORLD";
          case NEW_GEOMETRY: return "NEW_GEOMETRY";
          case NEW_GEOMETRIC_MODEL: return "NEW_GEOMETRIC_MODEL";
          case NEW_VOLUME: return "NEW_VOLUME";
          case NEW_VOLUMETRIC_MODEL: return "NEW_VOLUMETRIC_MODEL";
          case NEW_CAMERA: return "NEW_CAMERA";
          case NEW_TRANSFER_FUNCTION: return "NEW_TRANSFER_FUNCTION";
          case NEW_IMAGE_OP: return "NEW_IMAGE_OP";
          case NEW_MATERIAL: return "NEW_MATERIAL";
          case NEW_LIGHT: return "NEW_LIGHT";
          case NEW_DATA: return "NEW_DATA";
          case NEW_TEXTURE: return "NEW_TEXTURE";
          case NEW_GROUP: return "NEW_GROUP";
          case NEW_INSTANCE: return "NEW_INSTANCE";
          case COMMIT: return "COMMIT";
          case RELEASE: return "RELEASE";
          case LOAD_MODULE: return "LOAD_MODULE";
          case CREATE_FRAMEBUFFER: return "CREATE_FRAMEBUFFER";
          case MAP_FRAMEBUFFER: return "MAP_FRAMEBUFFER";
          case GET_VARIANCE: return "GET_VARIANCE";
          case RESET_ACCUMULATION: return "RESET_ACCUMULATION";
          case RENDER_FRAME_ASYNC: return "RENDER_FRAME_ASYNC";
          case SET_PARAM_OBJECT: return "SET_PARAM_OBJECT";
          case SET_PARAM_STRING: return "SET_PARAM_STRING";
          case SET_PARAM_INT: return "SET_PARAM_INT";
          case SET_PARAM_BOOL: return "SET_PARAM_BOOL";
          case SET_PARAM_FLOAT: return "SET_PARAM_FLOAT";
          case SET_PARAM_VEC2F: return "SET_PARAM_VEC2F";
          case SET_PARAM_VEC2I: return "SET_PARAM_VEC2I";
          case SET_PARAM_VEC3F: return "SET_PARAM_VEC3F";
          case SET_PARAM_VEC3I: return "SET_PARAM_VEC3I";
          case SET_PARAM_VEC4F: return "SET_PARAM_VEC4F";
          case SET_PARAM_VEC4I: return "SET_PARAM_VEC4I";
          case SET_PARAM_BOX1F: return "SET_PARAM_BOX1F";
          case SET_PARAM_BOX1I: return "SET_PARAM_BOX1I";
          case SET_PARAM_BOX2F: return "SET_PARAM_BOX2F";
          case SET_PARAM_BOX2I: return "SET_PARAM_BOX2I";
          case SET_PARAM_BOX3F: return "SET_PARAM_BOX3F";
          case SET_PARAM_BOX3I: return "SET_PARAM_BOX3I";
          case SET_PARAM_BOX4F: return "SET_PARAM_BOX4F";
          case SET_PARAM_BOX4I: return "SET_PARAM_BOX4I";
          case SET_PARAM_LINEAR3F: return "SET_PARAM_LINEAR3F";
          case SET_PARAM_AFFINE3F: return "SET_PARAM_AFFINE3F";
          case REMOVE_PARAM: return "REMOVE_PARAM";
          case SET_LOAD_BALANCER: return "SET_LOAD_BALANCER";
          case PICK: return "PICK";
          case FUTURE_IS_READY: return "FUTURE_IS_READY";
          case FUTURE_WAIT: return "FUTURE_WAIT";
          case FUTURE_CANCEL: return "FUTURE_CANCEL";
          case FUTURE_GET_PROGRESS: return "FUTURE_GET_PROGRESS";
          case FINALIZE: return "FINALIZE";
          case NONE:
          default: return "NONE/UNKNOWN/INVALID";
        }
      }

    }
  }
}

