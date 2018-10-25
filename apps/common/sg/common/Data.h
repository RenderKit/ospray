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

#pragma once

#include <iterator>
#include <functional>
// ospray::sg
#include "Node.h"
// ospcommon
#include "ospcommon/containers/AlignedVector.h"

namespace ospray {
  namespace sg {

    using intex_t = unsigned long long;

    struct OSPSG_INTERFACE DataBuffer : public Node
    {
      DataBuffer(OSPDataType type)
        : type(type)
      {
        Node::setType(toString());
      }

      virtual ~DataBuffer() override
      {
        ospRelease(data);
      }

      virtual std::string toString() const override
      { return "DataBuffer<abstract>"; }

      virtual void postCommit(RenderContext &) override
      {
        if (hasParent()) {
          if (parent().value().is<OSPObject>())
            ospSetData(parent().valueAs<OSPObject>(), name().c_str(), getOSP());
        }
      }

      void markAsModified() override
      {
        ospRelease(data);
        data = nullptr;

        Node::markAsModified();
      }

      template <typename T>
      T get(size_t idx) const { return ((T*)base())[idx]; }

      virtual void*  base()  const = 0;
      virtual size_t size()  const = 0;
      virtual size_t bytesPerElement() const = 0;

      template <typename T>
      inline T* baseAs() const { return static_cast<T*>(base()); }

      OSPDataType getType()  const { return type; }
      OSPData     getOSP()
      {
        if (!empty() && !data) {
          if (type == OSP_RAW) {
            data = ospNewData(numBytes(), type, base(),
                              OSP_DATA_SHARED_BUFFER);
          }
          else
            data = ospNewData(size(), type, base(), OSP_DATA_SHARED_BUFFER);

          ospCommit(data);
        }

        return data;
      }

      bool   empty()    const { return size() == 0; }
      size_t numBytes() const { return size() * bytesPerElement(); }

      OSPDataType type;
      OSPData     data {nullptr};

    protected:

      // Helper functions //

      std::string arrayTypeAsString() const;
    };

    // -------------------------------------------------------
    // data *ARRAYS*
    // -------------------------------------------------------
    template<typename T, int TID = OSPTypeFor<T>::value>
    struct DataArrayT : public DataBuffer
    {
      DataArrayT(T *base, size_t size, bool mine = true)
        : DataBuffer((OSPDataType)TID), numElements(size),
          mine(mine), base_ptr(base)
      {
        Node::setType(toString());

        deleter = [](T *p) { alignedFree(p); };
      }

      ~DataArrayT() override
      {
        if (mine && base_ptr)
          deleter(base_ptr);
      }

      std::string toString() const override
      { return "DataArray<" + arrayTypeAsString() + ">"; }

      void   *base() const override { return (void*)base_ptr; }
      size_t  size() const override { return numElements; }

      size_t bytesPerElement() const override { return sizeof(T); }

      inline T& operator[](int index) { return base_ptr[index]; }
      inline const T& operator[](int index) const { return base_ptr[index]; }

      // Data Members //

      using   ElementType = T;

      size_t  numElements {0};
      bool    mine {false};
      T      *base_ptr {nullptr};

      std::function<void(T*)> deleter;
    };

    using DataArray1uc = DataArrayT<unsigned char>;
    using DataArray1f  = DataArrayT<float>;
    using DataArray2f  = DataArrayT<vec2f>;
    using DataArray3f  = DataArrayT<vec3f>;
    using DataArray3fa = DataArrayT<vec3fa>;
    using DataArray4f  = DataArrayT<vec4f>;
    using DataArray1i  = DataArrayT<int>;
    using DataArray2i  = DataArrayT<vec2i>;
    using DataArray3i  = DataArrayT<vec3i>;
    using DataArray4i  = DataArrayT<vec4i>;
    using DataArrayOSP = DataArrayT<OSPObject>;
    using DataArrayRAW = DataArrayT<byte_t, OSP_RAW>;

    // -------------------------------------------------------
    // data *VECTORS*
    // -------------------------------------------------------
    template<typename T, int TID = OSPTypeFor<T>::value>
    struct DataVectorT : public DataBuffer
    {
      DataVectorT() : DataBuffer((OSPDataType)TID)
      {
        Node::setType(toString());
      }

      std::string toString() const override
      { return "DataVector<" + arrayTypeAsString() + ">"; }

      void   *base() const override { return (void*)v.data(); }
      size_t  size() const override { return v.size(); }

      size_t bytesPerElement() const override { return sizeof(T); }

      void push_back(const T &t) { v.push_back(t); }
      T& operator[](size_t index) { return v[index]; }
      const T& operator[](size_t index) const { return v[index]; }
      void resize(size_t n, T val=T()) { v.resize(n,val); }
      void clear() { v.resize(0); }


      // Data Members //

      using ElementType = T;

      containers::AlignedVector<T> v;
    };

    using DataVector1uc = DataVectorT<unsigned char>;
    using DataVector1f  = DataVectorT<float>;
    using DataVector2f  = DataVectorT<vec2f>;
    using DataVector3f  = DataVectorT<vec3f>;
    using DataVector3fa = DataVectorT<vec3fa>;
    using DataVector4f  = DataVectorT<vec4f>;
    using DataVector1i  = DataVectorT<int>;
    using DataVector2i  = DataVectorT<vec2i>;
    using DataVector3i  = DataVectorT<vec3i>;
    using DataVector4i  = DataVectorT<vec4i>;
    using DataVectorOSP = DataVectorT<OSPObject>;
    using DataVectorRAW = DataVectorT<byte_t, OSP_RAW>;
    using DataVectorAffine3f = DataVectorT<ospcommon::affine3f, OSP_RAW>;

    template <typename T, int TID = OSPTypeFor<T>::value>
    std::shared_ptr<DataBuffer> make_aligned_DataBuffer_node(void *data, size_t num)
    {
      using ARRAY_NODE_TYPE  = DataArrayT<T, TID>;
      using VECTOR_NODE_TYPE = DataVectorT<T, TID>;

      if ((size_t)data & 0x3) {
        // Data *not* aligned correctly, copy into an node with aligned memory
        auto node = std::make_shared<VECTOR_NODE_TYPE>();
        const auto *in_first = static_cast<const T*>(data);
        const auto *in_last  = in_first + num;
        std::copy(in_first, in_last, std::back_inserter(node->v));
        return std::move(node);
      } else {
        // Use the buffer directly (non-owning)
        return std::make_shared<ARRAY_NODE_TYPE>((T*)data, num, false);
      }
    }

  } // ::ospray::sg
} // ::ospray


