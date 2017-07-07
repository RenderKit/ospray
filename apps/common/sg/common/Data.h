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

#include "sg/common/TimeStamp.h"
#include "sg/common/Serialization.h"
#include "sg/common/RuntimeError.h"
// stl
#include <map>
// xml
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    typedef unsigned long long index_t;

#define INVALID_DATA_ERROR throw RuntimeError("invalid data format ")

    struct OSPSG_INTERFACE DataBuffer : public Node
    {
      DataBuffer(OSPDataType type)
        : type(type), data(nullptr)
      {}

      virtual ~DataBuffer() = default;

      virtual std::string toString() const override
      { return "DataBuffer<abstract>"; }

#if 0
      virtual float  get1f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec2f  get2f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3f  get3f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3fa get3fa(index_t idx) const { INVALID_DATA_ERROR; }
      virtual vec4f  get4f(index_t idx)  const { INVALID_DATA_ERROR; }

      virtual int    get1i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec2i  get2i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3i  get3i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec4i  get4i(index_t idx)  const { INVALID_DATA_ERROR; }
#else
      template <typename T>
      T get(index_t idx) const { return ((T*)base())[idx]; }
#endif

      virtual void       *base()  const = 0;
      virtual size_t      size()  const = 0;
      virtual bool        empty() const { return size() == 0; }
      virtual OSPDataType getType()  const { return type; }

      virtual OSPData     getOSP()
      {
        if (!empty() && !data) {
          data = ospNewData(size(), type, base(), OSP_DATA_SHARED_BUFFER);
          ospCommit(data);
        }
        return data;
      }

      OSPDataType type;
      OSPData     data;
    };

#undef INVALID_DATA_ERROR

    // -------------------------------------------------------
    // data *ARRAYS*
    // -------------------------------------------------------
    template<typename T, long int TID>
    struct DataArrayT : public DataBuffer
    {
      DataArrayT(T *base, size_t size, bool mine = true)
        : DataBuffer((OSPDataType)TID), numElements(size),
          mine(mine), base_ptr(base) {}
      ~DataArrayT() { if (mine && base_ptr) delete base_ptr; }

      void   *base() const override { return (void*)base_ptr; }
      size_t  size() const override { return numElements; }

      inline T& operator[](int index) { return base_ptr[index]; }
      inline const T& operator[](int index) const { return base_ptr[index]; }

      // Data Members //

      using   ElementType = T;

      size_t  numElements {0};
      bool    mine {false};
      T      *base_ptr {nullptr};
    };

#if 0
    struct DataArray1uc : public DataArrayT<unsigned char, OSP_UCHAR>
    {
      DataArray1uc(unsigned char *base, size_t size, bool mine=true)
        : DataArrayT<unsigned char,OSP_UCHAR>(base,size,mine)
      {}
    };

    struct DataArray2f : public DataArrayT<vec2f, OSP_FLOAT2>
    {
      DataArray2f(vec2f *base, size_t size, bool mine=true)
        : DataArrayT<vec2f,OSP_FLOAT2>(base,size,mine)
      {}
      virtual vec2f get2f(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray3f : public DataArrayT<vec3f, OSP_FLOAT3>
    {
      DataArray3f(vec3f *base, size_t size, bool mine=true)
        : DataArrayT<vec3f,OSP_FLOAT3>(base,size,mine)
      {}
      virtual vec3f get3f(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray3fa : public DataArrayT<vec3fa, OSP_FLOAT3A>
    {
      DataArray3fa(vec3fa *base, size_t size, bool mine=true)
        : DataArrayT<vec3fa,OSP_FLOAT3A>(base,size,mine)
      {}
      virtual vec3fa get3fa(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray4f : public DataArrayT<vec4f, OSP_FLOAT4>
    {
      DataArray4f(vec4f *base, size_t size, bool mine=true)
        : DataArrayT<vec4f,OSP_FLOAT4>(base,size,mine)
      {}
      virtual vec4f get4f(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray1i : public DataArrayT<int32_t, OSP_INT>
    {
      DataArray1i(int32_t *base, size_t size, bool mine=true)
        : DataArrayT<int32_t,OSP_INT>(base,size,mine)
        {}
      virtual int32_t get1i(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray3i : public DataArrayT<vec3i, OSP_INT3>
    {
      DataArray3i(vec3i *base, size_t size, bool mine=true)
        : DataArrayT<vec3i,OSP_INT3>(base,size,mine)
      {}
      virtual vec3i get3i(index_t idx) const override
      { return this->base_ptr[idx]; }
    };

    struct DataArray4i : public DataArrayT<vec4i, OSP_INT4>
    {
      DataArray4i(vec4i *base, size_t size, bool mine=true)
        : DataArrayT<vec4i,OSP_INT4>(base,size,mine)
      {}
      virtual vec4i get4i(index_t idx) const override
      { return this->base_ptr[idx]; }
    };
#else
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
#endif

    // -------------------------------------------------------
    // data *VECTORS*
    // -------------------------------------------------------
    template<typename T, int TID>
    struct DataVectorT : public DataBuffer
    {
      DataVectorT() : DataBuffer((OSPDataType)TID) {}
      void   *base() const override { return (void*)v.data(); }
      size_t  size() const override { return v.size(); }

      inline void push_back(const T &t) { v.push_back(t); }
      inline T& operator[](int index) { return v[index]; }
      inline const T& operator[](int index) const { return v[index]; }

      // Data Members //

      using ElementType = T;

      std::vector<T> v;
    };

#if 0
    struct DataVector2f : public DataVectorT<vec2f, OSP_FLOAT2>
    {
      virtual vec2f get2f(index_t idx) const override
      { assert(idx<v.size()); return this->v[idx]; }
    };

    struct DataVector3f : public DataVectorT<vec3f, OSP_FLOAT3>
    {
      virtual vec3f get3f(index_t idx) const override
      { assert(idx<v.size()); return this->v[idx]; }
    };

    struct DataVector3fa : public DataVectorT<vec3fa, OSP_FLOAT3A>
    {
      virtual vec3fa get3fa(index_t idx) const override
      { assert(idx<v.size()); return this->v[idx]; }
    };

    struct DataVector4f : public DataVectorT<vec4f, OSP_FLOAT4>
    {
      virtual vec4f get4f(index_t idx) const override
      { assert(idx<v.size()); return this->v[idx]; }
    };

    struct DataVector3i : public DataVectorT<vec3i, OSP_INT3>
    {
      virtual vec3i get3i(index_t idx) const override
      { assert(idx<v.size()); return this->v[idx]; }
    };
#else
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
#endif

    template<typename T>
    std::shared_ptr<T> make_aligned(void *data, size_t num)
    {
      typedef typename T::ElementType ElementType;
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


