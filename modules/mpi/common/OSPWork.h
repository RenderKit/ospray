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
#include "DataStreaming.h"
#include <map>

namespace ospray {
  namespace mpi {
    namespace work {

      /*! the command ID is a numerical value that corresponds to a
        given command type, and that lets the receiver figure out what
        kind of command it is to execute. CommandIDs are only useful
        inside \see Command structures.

        note the hardcoded IDs have no practical relevance; we use
        this only to more quickly identify the offending command if
        something went wrong and all we have is the last command id.
      */
      enum CommandTag
      {
        CMD_INVALID = -555,
        CMD_NEW_RENDERER=100,
        CMD_FRAMEBUFFER_CREATE,
        CMD_RENDER_FRAME,
        CMD_FRAMEBUFFER_CLEAR,
        CMD_FRAMEBUFFER_MAP,
        CMD_FRAMEBUFFER_UNMAP,

        CMD_NEW_DATA = 200,
        CMD_NEW_MODEL,
        CMD_NEW_GEOMETRY,
        CMD_NEW_MATERIAL,
        CMD_NEW_LIGHT,
        CMD_NEW_CAMERA,
        CMD_NEW_VOLUME,
        CMD_NEW_TRANSFERFUNCTION,
        CMD_NEW_TEXTURE2D,

        CMD_ADD_GEOMETRY=300,
        CMD_REMOVE_GEOMETRY,
        CMD_ADD_VOLUME,
        CMD_COMMIT,
        CMD_LOAD_MODULE,
        CMD_RELEASE,
        CMD_REMOVE_VOLUME,

        CMD_SET_MATERIAL=400,
        CMD_SET_REGION,
        CMD_SET_REGION_DATA,
        
        CMD_SET_OBJECT=500,
        CMD_SET_STRING,
        CMD_SET_INT,
        CMD_SET_FLOAT,
        CMD_SET_VEC2F,
        CMD_SET_VEC2I,
        CMD_SET_VEC3F,
        CMD_SET_VEC3I,
        CMD_SET_VEC4F,

        CMD_REMOVE_PARAM=250,

        CMD_SET_PIXELOP,
        CMD_NEW_PIXELOP,

        CMD_API_MODE,//TODO

        CMD_FINALIZE,
      };

      std::string commandTagToString(CommandTag tag);
      
      /*! abstract interface for a work item. a work item can
        serialize itself, de-serialize itself, and return a tag that
        allows the unbuffering code form figuring out what kind of
        work this is */
      struct Work
      {
        /*! type we use for representing tags */
        using tag_t = uint32_t;
        
        /*! return a tag that the buffering code can use to encode
          what kind of work this is */
        virtual tag_t getTag() const = 0;

        // /*! for debugging only - return some string of what this is */
        // virtual const char *toString()            const = 0;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const = 0;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) = 0;

        /*! returns whether this objects needs flushing of the buffered
         *  command stream */
        virtual bool flushing() { return false; }
        
        /*! what to do to execute this work item on a worker */
        virtual void run() {}

        /*! what to do to execute this work item on the master */
        virtual void runOnMaster() {}
      };

      using CreateWorkFct = std::shared_ptr<Work>(*)();

      void registerOSPWorkItems(std::map<Work::tag_t,CreateWorkFct> &workTypeRegistry);

      /*! templated base class that allows to implemnt common
        functoinality of a work item (name, tag, flush bit) though
        inheritance */
      template<int TAG>
      struct BaseWork : public Work
      {
        /*! return a tag that the buffering code can use to encode
          what kind of work this is */
        virtual Work::tag_t getTag() const override { return tag; }
        
        enum { tag = TAG };
      };


      /*! All of the simple CMD_NEW_* can be implemented with the same
       template. The more unique ones like NEW_DATA, NEW_TEXTURE2D or
       render specific objects like lights and materials require some
       more special treatment to handle sending the data or other
       params around as well. */
      template<typename T>
      struct NewObjectT : BaseWork<T::TAG>
      {
        NewObjectT() = default;
        NewObjectT(const char* type, ObjectHandle handle)
          : type(type), handle(handle) {}
        
        virtual void run()         override;
        virtual void runOnMaster() override;
        
        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override 
        { b << (int64)handle << type; }
        
        /*! de-serialize from a buffer that an object of this type ha
          serialized itself in */
        virtual void deserialize(ReadStream &b) override 
        { b >> handle.i64 >> type; }
        
        std::string  type;
        ObjectHandle handle;

        enum { tag = T::TAG };
      };

      /*! !{ speclialized NewObjectT into creating a light, model, etc object */
      struct ObjectType_Model             { enum { TAG = CMD_NEW_MODEL }; };
      struct ObjectType_Renderer          { enum { TAG = CMD_NEW_RENDERER }; };
      struct ObjectType_Camera            { enum { TAG = CMD_NEW_CAMERA }; };
      struct ObjectType_Volume            { enum { TAG = CMD_NEW_VOLUME }; };
      struct ObjectType_Geometry          { enum { TAG = CMD_NEW_GEOMETRY }; };
      struct ObjectType_PixelOp           { enum { TAG = CMD_NEW_PIXELOP }; };
      struct ObjectType_TransferFunction  { enum { TAG = CMD_NEW_TRANSFERFUNCTION }; };
      /*! @} */
      using NewModel            = NewObjectT<ObjectType_Model>;
      using NewPixelOp          = NewObjectT<ObjectType_PixelOp>;
      using NewRenderer         = NewObjectT<ObjectType_Renderer>;
      using NewCamera           = NewObjectT<ObjectType_Camera>;
      using NewVolume           = NewObjectT<ObjectType_Volume>;
      using NewGeometry         = NewObjectT<ObjectType_Geometry>;
      using NewTransferFunction = NewObjectT<ObjectType_TransferFunction>;
      

      struct NewMaterial : BaseWork<CMD_NEW_MATERIAL>
      {
        NewMaterial() = default;
        NewMaterial(const char* type, OSPRenderer renderer, ObjectHandle handle)
          : type(type), rendererHandle((ObjectHandle&)renderer), handle(handle)
        {}

        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)rendererHandle << (int64)handle << type; }
        
        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> rendererHandle.i64 >> handle.i64 >> type; }

        // const static size_t TAG = NewRendererObjectTag<T>::TAG;
        std::string  type;
        ObjectHandle rendererHandle;
        ObjectHandle handle;
      };

      struct NewLight : BaseWork<CMD_NEW_LIGHT>
      {
        NewLight() = default;
        NewLight(const char* type, OSPRenderer renderer, ObjectHandle handle)
          : type(type), rendererHandle((ObjectHandle&)renderer), handle(handle)
        {}
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)rendererHandle << (int64)handle << type; }
        
        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> rendererHandle.i64 >> handle.i64 >> type; }

        // const static size_t TAG = NewRendererObjectTag<T>::TAG;
        std::string  type;
        ObjectHandle rendererHandle;
        ObjectHandle handle;
      };

      // TODO: I don't do any of the sending back # of failures to
      // the "master", what would make sense to do in master/worker
      // and collaborative modes? Right now it will just throw and abort
      // template<>
      // void NewRendererObject<Material>::run();
      // template<>
      // void NewRendererObject<Light>::run();


      struct NewData : BaseWork<CMD_NEW_DATA>
      {
        NewData() = default;
        NewData(ObjectHandle handle, size_t nItems,
                OSPDataType format, void *initData, int flags);
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle       handle;
        size_t             nItems;
        OSPDataType        format;
        std::vector<char>  data;
        // If we're in collab/independent mode we can
        // be setting with just a local shared pointer
        void              *localData;
        int32              flags;
      };

      
      struct NewTexture2d : BaseWork<CMD_NEW_TEXTURE2D>
      {
        NewTexture2d() = default;
        NewTexture2d(ObjectHandle handle, vec2i dimensions,
                     OSPTextureFormat format, void *texture, uint32 flags);
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle      handle;
        vec2i             dimensions;
        OSPTextureFormat  format;
        std::vector<char> data;
        uint32            flags;
      };

      
      struct SetRegion : BaseWork<CMD_SET_REGION>
      {
        SetRegion() = default;
        SetRegion(OSPVolume volume, vec3i start, vec3i size, const void *src,
                  OSPDataType type);
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        vec3i regionStart, regionSize;
        OSPDataType type;
        std::vector<char> data;
      };

      struct CommitObject : BaseWork<CMD_COMMIT>
      {
        CommitObject() = default;
        CommitObject(ObjectHandle handle);
        
        virtual void run() override;
        // TODO: Which objects should the master commit?
        virtual void runOnMaster() override;

        virtual bool flushing() override { return true; }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
      };

      struct ClearFrameBuffer : BaseWork<CMD_FRAMEBUFFER_CLEAR>
      {
        ClearFrameBuffer() = default;
        ClearFrameBuffer(OSPFrameBuffer fb, uint32 channels);
        
        virtual void run() override;
        
        virtual void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        uint32 channels;
      };

      struct RenderFrame : BaseWork<CMD_RENDER_FRAME>
      {
        RenderFrame() = default;
        RenderFrame(OSPFrameBuffer fb, OSPRenderer renderer, uint32 channels);
        
        virtual void run() override;
        virtual void runOnMaster() override;
        virtual bool flushing() override { return true; }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle fbHandle;
        ObjectHandle rendererHandle;
        uint32 channels;
        // Variance result for adaptive accumulation
        float varianceResult {0.f};
      };

      struct AddVolume : BaseWork<CMD_ADD_VOLUME>
      {
        AddVolume() = default;
        AddVolume(OSPModel model, const OSPVolume &t)
          : modelHandle((const ObjectHandle&)model), objectHandle((const ObjectHandle&)t)
        {}
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }
        
        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      
      struct AddGeometry : BaseWork<CMD_ADD_GEOMETRY>
      {
        AddGeometry() = default;
        AddGeometry(OSPModel model, const OSPGeometry &t)
          : modelHandle((const ObjectHandle&)model), objectHandle((const ObjectHandle&)t)
        {}
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }
        
        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      
      struct RemoveGeometry : public BaseWork<CMD_REMOVE_GEOMETRY>
      {
        RemoveGeometry() = default;
        RemoveGeometry(OSPModel model, const OSPGeometry &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      struct RemoveVolume : public BaseWork<CMD_REMOVE_VOLUME>
      {
        RemoveVolume() = default;
        RemoveVolume(OSPModel model, const OSPVolume &t)
          : modelHandle((const ObjectHandle&)model),
            objectHandle((const ObjectHandle&)t)
        {}
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)modelHandle << (int64)objectHandle; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> modelHandle.i64 >> objectHandle.i64; }

        ObjectHandle modelHandle;
        ObjectHandle objectHandle;
      };

      struct CreateFrameBuffer : public BaseWork<CMD_FRAMEBUFFER_CREATE>
      {
        CreateFrameBuffer() = default;
        CreateFrameBuffer(ObjectHandle handle, vec2i dimensions,
                          OSPFrameBufferFormat format, uint32 channels);
        
        virtual void run() override;
        
        virtual void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        vec2i dimensions {-1};
        OSPFrameBufferFormat format;
        uint32 channels;
      };

      // Need specializations of the SetParam TAG for the actual
      // type that is being set so we can decode properly on the
      // other end.
      template<typename T>
      struct ParamTag;
      
      template<>
      struct ParamTag<std::string>
      {
        const static size_t TAG = CMD_SET_STRING;
      };
      
      template<>
      struct ParamTag<int>
      {
        const static size_t TAG = CMD_SET_INT;
      };
      
      template<>
      struct ParamTag<float>
      {
        const static size_t TAG = CMD_SET_FLOAT;
      };
      
      template<>
      struct ParamTag<vec2f>
      {
        const static size_t TAG = CMD_SET_VEC2F;
      };
      
      template<>
      struct ParamTag<vec2i>
      {
        const static size_t TAG = CMD_SET_VEC2I;
      };
      
      template<>
      struct ParamTag<vec3f>
      {
        const static size_t TAG = CMD_SET_VEC3F;
      };
      
      template<>
      struct ParamTag<vec3i>
      {
        const static size_t TAG = CMD_SET_VEC3I;
      };
      
      template<>
      struct ParamTag<vec4f>
      {
        const static size_t TAG = CMD_SET_VEC4F;
      };

      /*! this should go into implementation section ... */
      template<typename T>
      struct SetParam : BaseWork<ParamTag<T>::TAG>
      {
        SetParam() = default;
        
        SetParam(ObjectHandle handle, const char *name, const T &val)
          : handle(handle), name(name), val(val)
        {
          Assert(handle != nullHandle);
        }

        virtual void run()         override;
        virtual void runOnMaster() override;
        
        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)handle << name << val; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> handle.i64 >> name >> val; }

        const static size_t TAG = ParamTag<T>::TAG;
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
      struct SetMaterial : public BaseWork<CMD_SET_MATERIAL>
      {
        SetMaterial() = default;
        SetMaterial(ObjectHandle handle, OSPMaterial val)
          : handle(handle), material((ObjectHandle&)val)
        {
          Assert(handle != nullHandle);
          Assert(material != nullHandle);
        }
        
        virtual void run() override;
        
        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)handle << (int64)material; }

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> handle.i64 >> material.i64; }

        ObjectHandle handle;
        ObjectHandle material;
      };

      template<>
      struct SetParam<OSPObject> : BaseWork<CMD_SET_OBJECT>
      {
        SetParam() = default;
        
        SetParam(ObjectHandle handle, const char *name, OSPObject &obj)
          : handle(handle),
            name(name),
            val((ObjectHandle&)obj)
        {
          Assert(handle != nullHandle);
        }
        
        virtual void run() override
        {
          ManagedObject *obj = handle.lookup();
          Assert(obj);
          ManagedObject *param = NULL;
          if (val != NULL_HANDLE) {
            param = val.lookup();
            Assert(param);
          }
          obj->findParam(name.c_str(), true)->set(param);
        }
        
        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override
        { b << (int64)handle << name << (int64)val; }
        

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override
        { b >> handle.i64 >> name >> val.i64; }

        ObjectHandle handle;
        std::string name;
        ObjectHandle val;
      };

      struct RemoveParam : public BaseWork<CMD_REMOVE_PARAM>
      {
        RemoveParam() = default;
        RemoveParam(ObjectHandle handle, const char *name);
        
        virtual void run() override;
        virtual void runOnMaster() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
        std::string name;
      };

      struct SetPixelOp : public BaseWork<CMD_SET_PIXELOP>
      {
        SetPixelOp() = default;
        SetPixelOp(OSPFrameBuffer fb, OSPPixelOp op);
        
        virtual void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle fbHandle;
        ObjectHandle poHandle;
      };

      struct CommandRelease : BaseWork<CMD_RELEASE>
      {
        CommandRelease() = default;
        CommandRelease(ObjectHandle handle);
        void run() override;

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        ObjectHandle handle;
      };

      struct LoadModule : public BaseWork<CMD_LOAD_MODULE>
      {
        LoadModule() = default;
        LoadModule(const std::string &name);
        
        virtual void run() override;

        virtual bool flushing() override { return true; }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;

        std::string name;
      };

      struct CommandFinalize : public BaseWork<CMD_FINALIZE>
      {
        CommandFinalize() = default;

        virtual void run() override;
        virtual void runOnMaster() override;
        virtual bool flushing() override { return true; }

        /*! serializes itself on the given serial buffer - will write
          all data into this buffer in a way that it can afterwards
          un-serialize itself 'on the other side'*/
        virtual void serialize(WriteStream &b) const override;

        /*! de-serialize from a buffer that an object of this type has
          serialized itself in */
        virtual void deserialize(ReadStream &b) override;
      };
      
    } // ::ospray::mpi::work
  } // ::ospray::mpi
} // ::ospray
