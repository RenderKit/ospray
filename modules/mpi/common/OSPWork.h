// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

/*! \file OSPWork.h implements everything require to encode and
  serialize work items that represent api calls

  this code currently lives only in the mpi device, but shuld in
  theory also be applicable to other sorts of 'fabrics' for conveying
  such encoded work items
*/

#pragma once

#include <ospray/ospray.h>
#include "mpiCommon/MPICommon.h"
#include "common/ObjectHandle.h"

#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/utility/ArrayView.h"

#include "common/Model.h"
#include "render/Renderer.h"
#include "camera/Camera.h"
#include "volume/Volume.h"
#include "lights/Light.h"
#include "transferFunction/TransferFunction.h"

#include <map>

namespace ospray {
  namespace mpi {
    namespace work {

      using namespace mpicommon;
      using namespace ospcommon::networking;

      /*! abstract interface for a work item. a work item can
        serialize itself, de-serialize itself, and return a tag that
        allows the unbuffering code form figuring out what kind of
        work this is */
      struct Work
      {
        virtual ~Work() = default;
        /*! type we use for representing tags */
        using tag_t = size_t;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const = 0;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) = 0;

        /*! what to do to execute this work item on a worker */
        virtual void run() {}

        /*! what to do to execute this work item on the master */
        virtual void runOnMaster() {}
      };

      using CreateWorkFct    = std::unique_ptr<Work>(*)();
      using WorkTypeRegistry = std::map<Work::tag_t, CreateWorkFct>;

      /*! create a work unit of given type */
      template<typename T>
      inline std::unique_ptr<Work> make_work_unit()
      {
        return make_unique<T>();
      }

      template<typename T>
      inline CreateWorkFct createMakeWorkFct()
      {
        return make_work_unit<T>;
      }

      template <typename WORK_T>
      inline void registerWorkUnit(WorkTypeRegistry &registry)
      {
        static_assert(std::is_base_of<Work, WORK_T>::value,
                      "WORK_T must be a child class of ospray::work::Work!");

        registry[typeIdOf<WORK_T>()] = createMakeWorkFct<WORK_T>();
      }

      void registerOSPWorkItems(WorkTypeRegistry &registry);

      /*! this should go into implementation section ... */
      struct SetLoadBalancer :  public Work
      {
        SetLoadBalancer() = default;
        SetLoadBalancer(ObjectHandle _handle,
                        bool _useDynamicLoadBalancer,
                        int _numTilesPreAllocated = 4);

        void run() override;
        void runOnMaster() override;

        void serialize(WriteStream &b) const override;
        void deserialize(ReadStream &b) override;

      private:

        int   useDynamicLoadBalancer{false};
        int   numTilesPreAllocated{4};
        int64 handleID{-1};
      };

      /*! All of the simple CMD_NEW_* can be implemented with the same
       template. The more unique ones like NEW_DATA, NEW_TEXTURE2D or
       render specific objects like lights and materials require some
       more special treatment to handle sending the data or other
       params around as well. */
      template<typename T>
      struct NewObjectT : public Work
      {
        NewObjectT() = default;
        NewObjectT(const char* type, ObjectHandle handle)
          : type(type), handle(handle) {}

        void run() override
        {
          auto *obj = T::createInstance(type.c_str());
          handle.assign(obj);
        }

        void runOnMaster() override {}

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)handle << type; }

        /*! de-serialize from a buffer that an object of this type ha
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> type; }

        std::string  type;
        ObjectHandle handle;
      };

      // NewObjectT explicit instantiations ///////////////////////////////////

      using NewModel            = NewObjectT<Model>;
      using NewPixelOp          = NewObjectT<PixelOp>;
      using NewRenderer         = NewObjectT<Renderer>;
      using NewCamera           = NewObjectT<Camera>;
      using NewVolume           = NewObjectT<Volume>;
      using NewGeometry         = NewObjectT<Geometry>;
      using NewTransferFunction = NewObjectT<TransferFunction>;
      using NewTexture          = NewObjectT<Texture>;

      // Specializations for objects
      template<>
      void NewRenderer::runOnMaster();

      template<>
      void NewVolume::runOnMaster();

      template<>
      void NewModel::run();

      struct NewMaterial : public Work
      {
        NewMaterial() = default;
        NewMaterial(OSPRenderer renderer,
                    const char *material_type,
                    ObjectHandle handle)
          : rendererHandle((ObjectHandle&)renderer),
            materialType(material_type),
            handle(handle)
        {}

        void run() override;

        void serialize(WriteStream &b) const override
        { b << (int64)handle << (int64)rendererHandle << materialType; }

        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> rendererHandle.i64 >> materialType; }

        ObjectHandle rendererHandle;
        std::string  materialType;
        ObjectHandle handle;
      };

      struct NewMaterial2 : public Work
      {
        NewMaterial2() = default;
        NewMaterial2(const char *renderer_type,
                     const char *material_type,
                     ObjectHandle handle)
          : rendererType(renderer_type),
            materialType(material_type),
            handle(handle)
        {}

        void run() override;

        void serialize(WriteStream &b) const override
        { b << (int64)handle << rendererType << materialType; }

        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> rendererType >> materialType; }

        std::string  rendererType;
        std::string  materialType;
        ObjectHandle handle;
      };

      struct NewLight : public Work
      {
        NewLight() = default;
        NewLight(const char* type, ObjectHandle handle)
          : type(type), handle(handle)
        {}

        void run() override;

        void serialize(WriteStream &b) const override
        { b << (int64)handle << type; }

        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> type; }

        std::string  type;
        ObjectHandle handle;
      };

      struct NewData : public Work
      {
        NewData() = default;
        NewData(ObjectHandle handle, size_t nItems,
                OSPDataType format, const void *initData, int flags);

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        size_t       nItems;
        OSPDataType  format;

        std::vector<byte_t>        copiedData;
        utility::ArrayView<byte_t> dataView;//<-- may point to user data or
                                            //    'copiedData' member, depending
                                            //    on flags given on construction

        int32 flags;
      };

      struct SetRegion : public Work
      {
        SetRegion() = default;
        SetRegion(OSPVolume volume, vec3i start, vec3i size, const void *src,
                  OSPDataType type);

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle        handle;
        vec3i               regionStart;
        vec3i               regionSize;
        OSPDataType         type;
        std::vector<byte_t> data;
      };

      struct CommitObject : public Work
      {
        CommitObject() = default;
        CommitObject(ObjectHandle handle);

        void run() override;
        // TODO: Which objects should the master commit?
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
      };

      struct ClearFrameBuffer : public Work
      {
        ClearFrameBuffer() = default;
        ClearFrameBuffer(OSPFrameBuffer fb, uint32 channels);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        uint32 channels;
      };

      struct RenderFrame : public Work
      {
        RenderFrame() = default;
        RenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, uint32 channels);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle fbHandle;
        ObjectHandle rendererHandle;
        uint32 channels;
        // Variance result for adaptive accumulation
        float varianceResult {0.f};
      };

      struct AddVolume : public Work
      {
        AddVolume() = default;
        AddVolume(OSPModel model, const OSPVolume &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };


      struct AddGeometry : public Work
      {
        AddGeometry() = default;
        AddGeometry(OSPModel model, const OSPGeometry &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };


      struct RemoveGeometry : public Work
      {
        RemoveGeometry() = default;
        RemoveGeometry(OSPModel model, const OSPGeometry &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      struct RemoveVolume : public Work
      {
        RemoveVolume() = default;
        RemoveVolume(OSPModel model, const OSPVolume &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      struct CreateFrameBuffer : public Work
      {
        CreateFrameBuffer() = default;
        CreateFrameBuffer(ObjectHandle handle, vec2i dimensions,
                          OSPFrameBufferFormat format, uint32 channels);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        vec2i dimensions {-1};
        OSPFrameBufferFormat format;
        uint32 channels;
      };

      /*! this should go into implementation section ... */
      template<typename T>
      struct SetParam :  public Work
      {
        SetParam() = default;

        SetParam(ObjectHandle handle, const char *name, const T &val)
          : handle(handle), name(name), val(val)
        {
          Assert(handle != nullHandle);
        }

        inline void run() override
        {
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          obj->setParam(name, val);
        }

        void runOnMaster() override
        {
          if (!handle.defined())
            return;

          ManagedObject *obj = handle.lookup();
          if (dynamic_cast<Renderer*>(obj) || dynamic_cast<Volume*>(obj)) {
            obj->setParam(name, val);
          }
        }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)handle << name << val; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> name >> val; }

        ObjectHandle handle;
        std::string name;
        T val;
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
      struct SetMaterial : public Work
      {
        SetMaterial() = default;
        SetMaterial(ObjectHandle handle, OSPMaterial val)
          : handle(handle), material((ObjectHandle&)val)
        {
          Assert(handle != nullHandle);
          Assert(material != nullHandle);
        }

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)handle << (int64)material; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> material.i64; }

        ObjectHandle handle;
        ObjectHandle material;
      };

      template<>
      struct SetParam<OSPObject> : public Work
      {
        SetParam() = default;

        SetParam(ObjectHandle handle, const char *name, OSPObject &obj)
          : handle(handle),
            name(name),
            val((ObjectHandle&)obj)
        {
          Assert(handle != nullHandle);
        }

        void run() override
        {
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          ManagedObject *param = NULL;
          if (val != NULL_HANDLE) {
            param = val.lookup();
            Assert(param);
          }
          obj->setParam(name.c_str(), param);
        }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override
        { b << (int64)handle << name << (int64)val; }


        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override
        { b >> handle.i64 >> name >> val.i64; }

        ObjectHandle handle;
        std::string  name;
        ObjectHandle val;
      };

      struct RemoveParam : public Work
      {
        RemoveParam() = default;
        RemoveParam(ObjectHandle handle, const char *name);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        std::string name;
      };

      struct SetPixelOp : public Work
      {
        SetPixelOp() = default;
        SetPixelOp(OSPFrameBuffer fb, OSPPixelOp op);

        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle fbHandle;
        ObjectHandle poHandle;
      };

      struct CommandRelease : public Work
      {
        CommandRelease() = default;
        CommandRelease(ObjectHandle handle);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle handle;
      };

      struct LoadModule : public Work
      {
        LoadModule() = default;
        LoadModule(const std::string &name);

        void run() override;
        // We do need to load modules on master in the case of scripted modules
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        std::string name;
        int         errorCode {0};
      };

      struct CommandFinalize : public Work
      {
        CommandFinalize() = default;

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;
      };

      struct Pick : public Work
      {
        Pick() = default;
        Pick(OSPRenderer renderer, const vec2f &screenPos);

        void run() override;
        void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        void deserialize(ReadStream &b) override;

        ObjectHandle rendererHandle;
        vec2f screenPos;
        OSPPickResult pickResult;
      };

    } // ::ospray::mpi::work
  } // ::ospray::mpi
} // ::ospray
