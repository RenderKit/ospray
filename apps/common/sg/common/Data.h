// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "ospray/common/OSPCommon.h"
// ospray::sg
#include "sg/common/TimeStamp.h"
#include "sg/common/Serialization.h"
#include "sg/common/RuntimeError.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    using intex_t = unsigned long long;

    struct OSPSG_INTERFACE DataBuffer : public Node
    {
      DataBuffer(OSPDataType type)
        : type(type), data(nullptr)
      {}

      virtual ~DataBuffer() override = default;

      virtual std::string toString() const override
      { return "DataBuffer<abstract>"; }

      virtual void postCommit(RenderContext &) override
      {
        if (hasParent()) {
          if (parent().value().is<OSPObject>())
            ospSetData(parent().valueAs<OSPObject>(), name().c_str(), getOSP());
        }
      }

      template <typename T>
      T get(index_t idx) const { return ((T*)base())[idx]; }

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
      OSPData     data;
    };

    // -------------------------------------------------------
    // data *ARRAYS*
    // -------------------------------------------------------
    template<typename T, long int TID>
    struct DataArrayT : public DataBuffer
    {
      DataArrayT(T *base, size_t size, bool mine = true)
        : DataBuffer((OSPDataType)TID), numElements(size),
          mine(mine), base_ptr(base) {}
      ~DataArrayT() override { if (mine && base_ptr) delete base_ptr; }

      std::string toString() const override
      { return "DataArray<" + stringForType((OSPDataType)TID) + ">"; }

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
    };

    using DataArray1uc = DataArrayT<unsigned char, OSP_UCHAR>;
    using DataArray1f  = DataArrayT<float, OSP_FLOAT>;
    using DataArray2f  = DataArrayT<vec2f, OSP_FLOAT2>;
    using DataArray3f  = DataArrayT<vec3f, OSP_FLOAT3>;
    using DataArray3fa = DataArrayT<vec3fa, OSP_FLOAT3A>;
    using DataArray4f  = DataArrayT<vec4f, OSP_FLOAT4>;
    using DataArray1i  = DataArrayT<int, OSP_INT>;
    using DataArray2i  = DataArrayT<vec2i, OSP_INT2>;
    using DataArray3i  = DataArrayT<vec3i, OSP_INT3>;
    using DataArray4i  = DataArrayT<vec4i, OSP_INT4>;
    using DataArrayOSP = DataArrayT<OSPObject, OSP_OBJECT>;
    using DataArrayRAW = DataArrayT<byte_t, OSP_RAW>;

    // -------------------------------------------------------
    // data *VECTORS*
    // -------------------------------------------------------
    template<typename T, int TID>
    struct DataVectorT : public DataBuffer
    {
      DataVectorT() : DataBuffer((OSPDataType)TID) {}

      std::string toString() const override
      { return "DataVector<" + stringForType((OSPDataType)TID) + ">"; }

      void   *base() const override { return (void*)v.data(); }
      size_t  size() const override { return v.size(); }

      size_t bytesPerElement() const override { return sizeof(T); }

      void push_back(const T &t) { v.push_back(t); }
      T& operator[](int index) { return v[index]; }
      const T& operator[](int index) const { return v[index]; }

      // Data Members //

      using ElementType = T;

      std::vector<T> v;
    };

    using DataVector1uc = DataVectorT<unsigned char, OSP_UCHAR>;
    using DataVector1f  = DataVectorT<float, OSP_FLOAT>;
    using DataVector2f  = DataVectorT<vec2f, OSP_FLOAT2>;
    using DataVector3f  = DataVectorT<vec3f, OSP_FLOAT3>;
    using DataVector3fa = DataVectorT<vec3fa, OSP_FLOAT3A>;
    using DataVector4f  = DataVectorT<vec4f, OSP_FLOAT4>;
    using DataVector1i  = DataVectorT<int, OSP_INT>;
    using DataVector2i  = DataVectorT<vec2i, OSP_INT2>;
    using DataVector3i  = DataVectorT<vec3i, OSP_INT3>;
    using DataVector4i  = DataVectorT<vec4i, OSP_INT4>;
    using DataVectorOSP = DataVectorT<OSPObject, OSP_OBJECT>;
    using DataVectorRAW = DataVectorT<byte_t, OSP_RAW>;

    template<typename T>
    std::shared_ptr<T> make_shared_aligned(void *data, size_t num)
    {
      using ElementType = typename T::ElementType;
      if ((size_t)data & 0x3) {
        // Data *not* aligned correctly, copy into a new buffer appropriately...
        char *m = new char[num * sizeof(ElementType)];
        memcpy(m, data, num * sizeof(ElementType));
        return std::make_shared<T>((ElementType*)m, num, true);
      } else {
        return std::make_shared<T>((ElementType*)data, num, false);
      }
    }

  } // ::ospray::sg
} // ::ospray


