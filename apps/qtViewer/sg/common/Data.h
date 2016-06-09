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

    struct DataBuffer : public Node {
      DataBuffer(OSPDataType type)
        : type(type), data(NULL)
      {}
      virtual ~DataBuffer() {}

      virtual std::string toString() const { return "DataBuffer<abstract>"; };

      virtual float  get1f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec2f  get2f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3f  get3f(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3fa get3fa(index_t idx) const { INVALID_DATA_ERROR; }
      virtual vec4f  get4f(index_t idx)  const { INVALID_DATA_ERROR; }

      virtual int    get1i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec2i  get2i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec3i  get3i(index_t idx)  const { INVALID_DATA_ERROR; }
      virtual vec4i  get4i(index_t idx)  const { INVALID_DATA_ERROR; }

      virtual void       *getBase()  const = 0;
      virtual size_t      getSize()  const = 0;
      virtual bool        notEmpty() const { return getSize() != 0; };
      virtual OSPDataType getType()  const { return type; };

      virtual OSPData     getOSP()   {
        if (notEmpty() && !data) {
          data = ospNewData(getSize(),type,getBase(),OSP_DATA_SHARED_BUFFER);
          ospCommit(data);
        }
        return data;
      };

      OSPData     data;
      OSPDataType type;
    };

#undef INVALID_DATA_ERROR


    // -------------------------------------------------------
    // data *ARRAYS*
    // -------------------------------------------------------
    template<typename T, long int TID>
    struct DataArrayT : public DataBuffer {
      DataArrayT(T *base, size_t size, bool mine=true)
        : DataBuffer((OSPDataType)TID), base(base), mine(mine), size(size)
      {}
      virtual void        *getBase()  const { return (void*)base; }
      virtual size_t       getSize()  const { return size; }
      virtual ~DataArrayT() { if (mine && base) delete base; }

      typedef T ElementType;

      size_t size;
      bool   mine;
      T     *base;
    };

    struct DataArray2f : public DataArrayT<vec2f,OSP_FLOAT2> {
      DataArray2f(vec2f *base, size_t size, bool mine=true)
        : DataArrayT<vec2f,OSP_FLOAT2>(base,size,mine)
      {}
      virtual vec2f  get2f(index_t idx)  const { return this->base[idx]; }
    };
    struct DataArray3f : public DataArrayT<vec3f,OSP_FLOAT3> {
      DataArray3f(vec3f *base, size_t size, bool mine=true)
        : DataArrayT<vec3f,OSP_FLOAT3>(base,size,mine)
      {}
      virtual vec3f  get3f(index_t idx)  const { return this->base[idx]; }
    };
    struct DataArray3fa : public DataArrayT<vec3fa,OSP_FLOAT3A> {
      DataArray3fa(vec3fa *base, size_t size, bool mine=true)
        : DataArrayT<vec3fa,OSP_FLOAT3A>(base,size,mine)
      {}
      virtual vec3fa  get3fa(index_t idx)  const { return this->base[idx]; }
    };

    struct DataArray1i : public DataArrayT<int32_t,OSP_INT> {
      DataArray1i(int32_t *base, size_t size, bool mine=true)
        : DataArrayT<int32_t,OSP_INT>(base,size,mine)
        {}
      virtual int32_t get1i(index_t idx) const { return this->base[idx]; }
    };

    struct DataArray3i : public DataArrayT<vec3i,OSP_INT3> {
      DataArray3i(vec3i *base, size_t size, bool mine=true)
        : DataArrayT<vec3i,OSP_INT3>(base,size,mine)
      {}
      virtual vec3i  get3i(index_t idx)  const { return this->base[idx]; }
    };
    struct DataArray4i : public DataArrayT<vec4i,OSP_INT4> {
      DataArray4i(vec4i *base, size_t size, bool mine=true)
        : DataArrayT<vec4i,OSP_INT4>(base,size,mine)
      {}
      virtual vec4i  get4i(index_t idx)  const { return this->base[idx]; }
    };

    // -------------------------------------------------------
    // data *VECTORS*
    // -------------------------------------------------------
    template<typename T, int TID>
    struct DataVectorT : public DataBuffer {
      DataVectorT()
        : DataBuffer((OSPDataType)TID)
      {}
      virtual void       *getBase()  const { return (void*)&v[0]; }
      virtual size_t      getSize()  const { return v.size(); }
      inline void push_back(const T &t) { v.push_back(t); }
      std::vector<T> v;
    };

    struct DataVector2f : public DataVectorT<vec2f,OSP_FLOAT2> {
      virtual vec2f  get2f(index_t idx)  const { return this->v[idx]; }
    };
    struct DataVector3f : public DataVectorT<vec3f,OSP_FLOAT3> {
      virtual vec3f  get3f(index_t idx)  const { return this->v[idx]; }
    };
    struct DataVector3fa : public DataVectorT<vec3fa,OSP_FLOAT3A> {
      virtual vec3fa  get3fa(index_t idx)  const { return this->v[idx]; }
    };
    struct DataVector3i : public DataVectorT<vec3i,OSP_INT3> {
      virtual vec3i  get3i(index_t idx)  const { return this->v[idx]; }
    };

    template<typename T>
    T* make_aligned(void *data, size_t num)
    {
      typedef typename T::ElementType ElementType;
      if ((size_t)data & 0x3) {
        // Data *not* aligned correctly, copy into a new buffer appropriately...
        char *m = new char[num * sizeof(ElementType)];
        memcpy(m, data, num * sizeof(ElementType));
        return new T((ElementType*)m, num, true);
      } else {
        return new T((ElementType*)data, num, false);
      }
    }

  } // ::ospray::sg
} // ::ospray


