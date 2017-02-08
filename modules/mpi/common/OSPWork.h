// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#include "mpiCommon/MPICommon.h"
#include "mpiCommon/command.h"
#include "mpiCommon/SerialBuffer.h"
#include "common/Model.h"
#include "common/Data.h"
#include "common/Library.h"
#include "common/Model.h"
#include "geometry/TriangleMesh.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "lights/Light.h"
#include "texture/Texture2D.h"
#include "transferFunction/TransferFunction.h"
#include "common/ObjectHandle.h"

namespace ospray {
  namespace mpi {
    namespace work {

      void initWorkMap();

      template<typename T>
      struct NewObjectTag;
      template<>
      struct NewObjectTag<Renderer> {
        const static size_t TAG = CMD_NEW_RENDERER;
      };
      template<>
      struct NewObjectTag<Model> {
        const static size_t TAG = CMD_NEW_MODEL;
      };
      template<>
      struct NewObjectTag<Geometry> {
        const static size_t TAG = CMD_NEW_GEOMETRY;
      };
      template<>
      struct NewObjectTag<Camera> {
        const static size_t TAG = CMD_NEW_CAMERA;
      };
      template<>
      struct NewObjectTag<Volume> {
        const static size_t TAG = CMD_NEW_VOLUME;
      };
      template<>
      struct NewObjectTag<TransferFunction> {
        const static size_t TAG = CMD_NEW_TRANSFERFUNCTION;
      };
      template<>
      struct NewObjectTag<PixelOp> {
        const static size_t TAG = CMD_NEW_PIXELOP;
      };

      // All of the simple CMD_NEW_* can be implemented with the same
      // template. The more unique ones like NEW_DATA, NEW_TEXTURE2D
      // or render specific objects like lights and materials require
      // some more special treatment to handle sending the data or
      // other params around as well.
      template<typename T>
      struct NewObject : Work {
        const static size_t TAG = NewObjectTag<T>::TAG;
        std::string type;
        ObjectHandle handle;

        NewObject() {}
        NewObject(const char* type, ObjectHandle handle) : type(type), handle(handle) {}
        void run() override {}
        void runOnMaster() override {}
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)handle << type;
        }
        void deserialize(SerialBuffer &b) override {
          b >> handle.i64 >> type;
        }
      };
      // Specializations of the run method for actually creating the
      // new objects of each type.
      template<>
      void NewObject<Renderer>::run();
      // For the renderer stored error threshold we need to make
      // the renderer on the master as well.
      template<>
      void NewObject<Renderer>::runOnMaster();
      template<>
      void NewObject<Model>::run();
      template<>
      void NewObject<Geometry>::run();
      template<>
      void NewObject<Camera>::run();
      template<>
      void NewObject<Volume>::run();
      // We need to make volumes on master so we can set the string param
      template<>
      void NewObject<Volume>::runOnMaster();
      template<>
      void NewObject<TransferFunction>::run();
      template<>
      void NewObject<PixelOp>::run();

      template<typename T>
      struct NewRendererObjectTag;
      template<>
      struct NewRendererObjectTag<Material> {
        const static size_t TAG = CMD_NEW_MATERIAL;
      };
      template<>
      struct NewRendererObjectTag<Light> {
        const static size_t TAG = CMD_NEW_LIGHT;
      };
      template<typename T>
      struct NewRendererObject : Work {
        const static size_t TAG = NewRendererObjectTag<T>::TAG;
        std::string type;
        ObjectHandle rendererHandle;
        ObjectHandle handle;

        NewRendererObject() {}
        NewRendererObject(const char* type, OSPRenderer renderer, ObjectHandle handle)
          : type(type), rendererHandle((ObjectHandle&)renderer), handle(handle)
        {}
        void run() override {}
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)rendererHandle << (int64)handle << type;
        }
        void deserialize(SerialBuffer &b) override {
          b >> rendererHandle.i64 >> handle.i64 >> type;
        }
      };
      // TODO: I don't do any of the sending back # of failures to
      // the "master", what would make sense to do in master/worker
      // and collaborative modes? Right now it will just throw and abort
      template<>
      void NewRendererObject<Material>::run();
      template<>
      void NewRendererObject<Light>::run();

      struct NewData : Work {
        const static size_t TAG = CMD_NEW_DATA;
        ObjectHandle handle;
        size_t nItems;
        OSPDataType format;
        std::vector<char> data;
        // If we're in collab/independent mode we can
        // be setting with just a local shared pointer
        void *localData;
        int32 flags;

        NewData();
        NewData(ObjectHandle handle, size_t nItems,
            OSPDataType format, void *initData, int flags);
        void run() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct NewTexture2d : Work {
        const static size_t TAG = CMD_NEW_TEXTURE2D;
        ObjectHandle handle;
        vec2i dimensions;
        OSPTextureFormat format;
        std::vector<char> data;
        uint32 flags;

        NewTexture2d();
        NewTexture2d(ObjectHandle handle, vec2i dimensions,
            OSPTextureFormat format, void *texture, uint32 flags);
        void run() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct SetRegion : Work {
        const static size_t TAG = CMD_SET_REGION;
        ObjectHandle handle;
        vec3i regionStart, regionSize;
        OSPDataType type;
        std::vector<char> data;

        SetRegion();
        SetRegion(OSPVolume volume, vec3i start, vec3i size, const void *src,
                  OSPDataType type);
        void run() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct CommitObject : Work {
        const static size_t TAG = CMD_COMMIT;
        ObjectHandle handle;

        CommitObject();
        CommitObject(ObjectHandle handle);
        void run() override;
        // TODO: Which objects should the master commit?
        void runOnMaster() override;
        size_t getTag() const override;
        bool flushing() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct ClearFrameBuffer : Work {
        const static size_t TAG = CMD_FRAMEBUFFER_CLEAR;
        ObjectHandle handle;
        uint32 channels;

        ClearFrameBuffer();
        ClearFrameBuffer(OSPFrameBuffer fb, uint32 channels);
        void run() override;
        void runOnMaster() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct RenderFrame : Work {
        const static size_t TAG = CMD_RENDER_FRAME;
        ObjectHandle fbHandle;
        ObjectHandle rendererHandle;
        uint32 channels;
        // Variance result for adaptive accumulation
        float varianceResult;

        RenderFrame();
        RenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, uint32 channels);
        void run() override;
        void runOnMaster() override;
        size_t getTag() const override;
        bool flushing() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      template<typename T>
      struct AddObjectTag;
      template<>
      struct AddObjectTag<OSPGeometry> {
        const static size_t TAG = CMD_ADD_GEOMETRY;
      };
      template<>
      struct AddObjectTag<OSPVolume> {
        const static size_t TAG = CMD_ADD_VOLUME;
      };

      template<typename T>
      struct AddObject : Work {
        const static size_t TAG = AddObjectTag<T>::TAG;
        ObjectHandle modelHandle;
        ObjectHandle objectHandle;

        AddObject(){}
        AddObject(OSPModel model, const T &t)
          : modelHandle((const ObjectHandle&)model), objectHandle((const ObjectHandle&)t)
        {}
        void run() override {}
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)modelHandle << (int64)objectHandle;
        }
        void deserialize(SerialBuffer &b) override {
          b >> modelHandle.i64 >> objectHandle.i64;
        }
      };
      template<>
      void AddObject<OSPGeometry>::run();
      template<>
      void AddObject<OSPVolume>::run();

      template<typename T>
      struct RemoveObjectTag;
      template<>
      struct RemoveObjectTag<OSPGeometry> {
        const static size_t TAG = CMD_REMOVE_GEOMETRY;
      };
      template<>
      struct RemoveObjectTag<OSPVolume> {
        const static size_t TAG = CMD_REMOVE_VOLUME;
      };

      template<typename T>
      struct RemoveObject : Work {
        const static size_t TAG = RemoveObjectTag<T>::TAG;
        ObjectHandle modelHandle;
        ObjectHandle objectHandle;

        RemoveObject(){}
        RemoveObject(OSPModel model, const T &t)
          : modelHandle((const ObjectHandle&)model), objectHandle((const ObjectHandle&)t)
        {}
        void run() override {}
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)modelHandle << (int64)objectHandle;
        }
        void deserialize(SerialBuffer &b) override {
          b >> modelHandle.i64 >> objectHandle.i64;
        }
      };
      template<>
      void RemoveObject<OSPGeometry>::run();
      template<>
      void RemoveObject<OSPVolume>::run();

      struct CreateFrameBuffer : Work {
        const static size_t TAG = CMD_FRAMEBUFFER_CREATE;
        ObjectHandle handle;
        vec2i dimensions;
        OSPFrameBufferFormat format;
        uint32 channels;

        CreateFrameBuffer();
        CreateFrameBuffer(ObjectHandle handle, vec2i dimensions,
            OSPFrameBufferFormat format, uint32 channels);
        void run() override;
        void runOnMaster() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      // Need specializations of the SetParam TAG for the actual
      // type that is being set so we can decode properly on the
      // other end.
      template<typename T>
      struct ParamTag;
      template<>
      struct ParamTag<std::string> {
        const static size_t TAG = CMD_SET_STRING;
      };
      template<>
      struct ParamTag<int> {
        const static size_t TAG = CMD_SET_INT;
      };
      template<>
      struct ParamTag<float> {
        const static size_t TAG = CMD_SET_FLOAT;
      };
      template<>
      struct ParamTag<vec2f> {
        const static size_t TAG = CMD_SET_VEC2F;
      };
      template<>
      struct ParamTag<vec2i> {
        const static size_t TAG = CMD_SET_VEC2I;
      };
      template<>
      struct ParamTag<vec3f> {
        const static size_t TAG = CMD_SET_VEC3F;
      };
      template<>
      struct ParamTag<vec3i> {
        const static size_t TAG = CMD_SET_VEC3I;
      };
      template<>
      struct ParamTag<vec4f> {
        const static size_t TAG = CMD_SET_VEC4F;
      };

      template<typename T>
      struct SetParam : Work {
        const static size_t TAG = ParamTag<T>::TAG;
        ObjectHandle handle;
        std::string name;
        T val;

        SetParam(){}
        SetParam(ObjectHandle handle, const char *name, const T &val)
          : handle(handle), name(name), val(val)
        {
          Assert(handle != nullHandle);
        }
        void run() override {
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->findParam(name.c_str(), true)->set(val);
        }
        void runOnMaster() override {
          ManagedObject *obj = handle.lookup();
          if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
            obj->findParam(name.c_str(), true)->set(val);
          }
        }
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)handle << name << val;
        }
        void deserialize(SerialBuffer &b) override {
          b >> handle.i64 >> name >> val;
        }
      };
      // run for setString needs to know to pass the C string to
      // set the param so we need to provide a different run.
      template<>
      void SetParam<std::string>::run();
      template<>
      void SetParam<std::string>::runOnMaster();

      // both SetMaterial and SetObject take more different forms than the other
      // set operations since it doesn't take a name at all so
      // we go through a full specializations for them.
      template<>
      struct SetParam<OSPMaterial> : Work {
        const static size_t TAG = CMD_SET_MATERIAL;
        ObjectHandle handle;
        ObjectHandle material;

        SetParam(){}
        SetParam(ObjectHandle handle, OSPMaterial val)
          : handle(handle), material((ObjectHandle&)val)
        {
          Assert(handle != nullHandle);
          Assert(material != nullHandle);
        }
        void run() override {
          Geometry *obj = (Geometry*)handle.lookup();
          Material *mat = (Material*)material.lookup();
          Assert(obj);
          Assert(mat);
          obj->setMaterial(mat);
        }
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)handle << (int64)material;
        }
        void deserialize(SerialBuffer &b) override {
          b >> handle.i64 >> material.i64;
        }
      };

      template<>
      struct SetParam<OSPObject> : Work {
        const static size_t TAG = CMD_SET_OBJECT;
        ObjectHandle handle;
        std::string name;
        ObjectHandle val;

        SetParam(){}
        SetParam(ObjectHandle handle, const char *name, OSPObject &obj)
          : handle(handle), name(name), val((ObjectHandle&)obj)
        {
          Assert(handle != nullHandle);
        }
        void run() override {
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          ManagedObject *param = NULL;
          if (val != NULL_HANDLE) {
            param = val.lookup();
            Assert(param);
          }
          obj->findParam(name.c_str(), true)->set(param);
        }
        size_t getTag() const override {
          return TAG;
        }
        void serialize(SerialBuffer &b) const override {
          b << (int64)handle << name << (int64)val;
        }
        void deserialize(SerialBuffer &b) override {
          b >> handle.i64 >> name >> val.i64;
        }
      };

      struct RemoveParam : Work {
        const static size_t TAG = CMD_REMOVE_PARAM;
        ObjectHandle handle;
        std::string name;

        RemoveParam();
        RemoveParam(ObjectHandle handle, const char *name);
        void run() override;
        void runOnMaster() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct SetPixelOp : Work {
        const static size_t TAG = CMD_SET_PIXELOP;
        ObjectHandle fbHandle;
        ObjectHandle poHandle;

        SetPixelOp();
        SetPixelOp(OSPFrameBuffer fb, OSPPixelOp op);
        void run() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct CommandRelease : Work {
        const static size_t TAG = CMD_RELEASE;
        ObjectHandle handle;

        CommandRelease();
        CommandRelease(ObjectHandle handle);
        void run() override;
        size_t getTag() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct LoadModule : Work {
        const static size_t TAG = CMD_LOAD_MODULE;
        std::string name;

        LoadModule();
        LoadModule(const std::string &name);
        void run() override;
        size_t getTag() const override;
        bool flushing() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };

      struct CommandFinalize : Work {
        const static size_t TAG = CMD_FINALIZE;

        CommandFinalize();
        void run() override;
        void runOnMaster() override;
        size_t getTag() const override;
        bool flushing() const override;
        void serialize(SerialBuffer &b) const override;
        void deserialize(SerialBuffer &b) override;
      };
    }// namespace work
  }// namespace mpi
}// namespace ospray
