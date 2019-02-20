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
#include <sstream>
#include <vector>
#include "../common.h"
#include "../range.h"
#include "for_each.h"

namespace ospcommon {
  namespace array3D {

    /*! ABSTRACTION for a 3D array of data */
    template <typename value_t>
    struct Array3D
    {
      virtual ~Array3D() = default;

      /*! return size (ie, "dimensions") of volume */
      virtual vec3i size() const = 0;

      /*! get cell value at given location, but ensure that location
        is actually a valid cell ID inside the volume (clamps to
        nearest cell in volume if 'where' is outside) */
      virtual value_t get(const vec3i &where) const = 0;

      /*! get the range/interval of all cell values in the given
        begin/end region of the volume */
      range_t<value_t> getValueRange(const vec3i &begin,
                                     const vec3i &end) const
      {
        range_t<value_t> v = get(begin);
        for_each(begin, end, [&](const vec3i &idx) { v.extend(get(idx)); });
        return v;
      }

      /*! get value range over entire volume */
      range_t<value_t> getValueRange() const
      {
        return getValueRange(vec3i(0), size());
      }

      /*! returns number of elements (as 64-bit int) across all dimensions */
      virtual size_t numElements() const = 0;
    };

    /*! implementation for an actual array3d that stores a 3D array of values */
    template <typename value_t>
    struct ActualArray3D : public Array3D<value_t>
    {
      ActualArray3D(const vec3i &dims, void *externalMem = nullptr);
      ~ActualArray3D() override
      {
        if (valuesAreMine)
          delete[] value;
      }

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      value_t get(const vec3i &where) const override;

      /*! set cell value at location to given value

        \warning 'where' MUST be a valid cell location */
      void set(const vec3i &where, const value_t &t);

      void clear(const value_t &t);

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override;

      /* compute the (1D) linear array index for a (3D) grid coordinate */
      size_t indexOf(const vec3i &pos) const
      {
        return pos.x + size_t(dims.x) * (pos.y + size_t(dims.y) * pos.z);
      }

      const vec3i dims;
      value_t *value;
      // bool that specified whether it was us that alloc'ed this mem,
      // and thus, whether we should free it upon termination.
      bool valuesAreMine;
    };

    /*! shifts another array3d by a given amount */
    template <typename value_t>
    struct IndexShiftedArray3D : public Array3D<value_t>
    {
      IndexShiftedArray3D(std::shared_ptr<Array3D<value_t>> _actual,
                          const vec3i &_shift)
          : actual(_actual), shift(_shift)
      {
      }

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override
      {
        return actual->size();
      }

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      value_t get(const vec3i &where) const override
      {
        return actual->get((where + size() + shift) % size());
      }

      /*! set cell value at location to given value

        \warning 'where' MUST be a valid cell location */
      void set(const vec3i &, const value_t &)
      {
        throw std::runtime_error("cannot 'set' in a IndexShiftArray3D");
      }

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override
      {
        return actual->numElements();
      }

      const vec3i shift;
      const std::shared_ptr<Array3D<value_t>> actual;
    };

    /*! implemnetaiton of a wrapper class that makes an actual array3d
      of one type look like that of another type */
    template <typename in_t, typename out_t>
    struct Array3DAccessor : public Array3D<out_t>
    {
      Array3DAccessor(std::shared_ptr<Array3D<in_t>> actual);

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      out_t get(const vec3i &where) const override;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override;

     private:
      //! the actual 3D array we're wrapping around
      const std::shared_ptr<Array3D<in_t>> actual;
    };

    /*! wrapper class that generates an artificially larger data set by
      simply repeating the given input */
    template <typename T>
    struct Array3DRepeater : public Array3D<T>
    {
      Array3DRepeater(const std::shared_ptr<Array3D<T>> &actual,
                      const vec3i &repeatedSize);

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override;

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      T get(const vec3i &where) const override;

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override;

      const vec3i repeatedSize;
      const std::shared_ptr<Array3D<T>> actual;
    };

    /*! implements a sub-set of another array3d */
    template <typename value_t>
    struct SubBoxArray3D : public Array3D<value_t>
    {
      SubBoxArray3D(const std::shared_ptr<Array3D<value_t>> &actual,
                    const box3i &clipBox)
        : clipBox(clipBox), actual(actual)
      {
        assert(actual);
        assert(clipBox.upper.x <= actual->size().x);
        assert(clipBox.upper.y <= actual->size().y);
        assert(clipBox.upper.z <= actual->size().z);
      }

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override
      {
        return clipBox.size();
      }

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      value_t get(const vec3i &where) const override
      {
        return actual->get(where + clipBox.lower);
      }

      /*! set cell value at location to given value

        \warning 'where' MUST be a valid cell location */
      void set(const vec3i &, const value_t &)
      {
        throw std::runtime_error("cannot 'set' in a SubBoxArray3D");
      }

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override
      {
        vec3i dims = clipBox.size();
        return size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
      }

      const box3i clipBox;
      const std::shared_ptr<Array3D<value_t>> actual;
    };

    /*! implements a array3d that's composed of multiple individual slices */
    template <typename value_t>
    struct MultiSliceArray3D : public Array3D<value_t>
    {
      MultiSliceArray3D(
          const std::vector<std::shared_ptr<Array3D<value_t>>> &slice)
          : slice(slice)
      {
      }

      /*! return size (ie, "dimensions") of volume */
      vec3i size() const override
      {
        return vec3i(slice[0]->size().x, slice[0]->size().y, slice.size());
      }

      /*! get cell value at location

        \warning 'where' MUST be a valid cell location */
      value_t get(const vec3i &where) const override
      {
        return slice[clamp(where.z, 0, (int)slice.size() - 1)]->get(
            vec3i(where.x, where.y, 0));
      }

      /*! set cell value at location to given value

        \warning 'where' MUST be a valid cell location */
      void set(const vec3i &, const value_t &)
      {
        throw std::runtime_error("cannot 'set' in a MultiSliceArray3D");
      }

      /*! returns number of elements (as 64-bit int) across all dimensions */
      size_t numElements() const override
      {
        return slice[0]->numElements() * slice.size();
      }

      const std::vector<std::shared_ptr<Array3D<value_t>>> slice;
    };

#ifndef _WIN32
    /*! load raw file with given dimensions. the 'type' of the raw
      file (uint8,float,...) is given through the function's
      template parameter */
    template <typename T>
    std::shared_ptr<Array3D<T>> OSPCOMMON_INTERFACE
    loadRAW(const std::string &fileName, const vec3i &dims);

    /*! load raw file with given dimensions. the 'type' of the raw
      file (uint8,float,...) is given through the function's
      template parameter */
    template <typename T>
    std::shared_ptr<Array3D<T>> OSPCOMMON_INTERFACE
    mmapRAW(const std::string &fileName, const vec3i &dims);
#endif

    // Inlined definitions ////////////////////////////////////////////////////

    // ActualArray3D //

    template <typename T>
    inline vec3i ActualArray3D<T>::size() const
    {
      return dims;
    }

    template <typename T>
    inline T ActualArray3D<T>::get(const vec3i &_where) const
    {
      assert(value != nullptr);
      const vec3i where = max(vec3i(0), min(_where, dims - vec3i(1)));
      size_t index =
          where.x + size_t(dims.x) * (where.y + size_t(dims.y) * (where.z));
      assert(value);
      assert(index < numElements());
      const T v = value[index];
      return v;
    }

    template <typename T>
    inline size_t ActualArray3D<T>::numElements() const
    {
      return size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
    }

    template <typename T>
    inline ActualArray3D<T>::ActualArray3D(const vec3i &dims, void *externalMem)
        : dims(dims),
          value((T *)externalMem),
          valuesAreMine(externalMem == nullptr)
    {
      try {
        if (!value) {
          const size_t numVoxels = longProduct(dims);
          value = new T[numVoxels];
        }
      } catch (const std::bad_alloc &) {
        std::stringstream ss;
        ss << "could not allocate memory for Array3D of dimensions " << dims
           << " (in Array3D::Array3D())";
        throw std::runtime_error(ss.str());
      }
    }

    template <typename T>
    inline void ActualArray3D<T>::set(const vec3i &where, const T &t)
    {
      value[longIndex(where, size())] = t;
    }

    template <typename T>
    inline void ActualArray3D<T>::clear(const T &t)
    {
      for_each(size(), [&](const vec3i &idx){ set(idx, t); });
    }

    // Array3DAccessor //

    template <typename in_t, typename out_t>
    inline Array3DAccessor<in_t, out_t>::Array3DAccessor(
        std::shared_ptr<Array3D<in_t>> actual)
        : actual(actual)
    {
    }

    template <typename in_t, typename out_t>
    inline vec3i Array3DAccessor<in_t, out_t>::size() const
    {
      return actual->size();
    }

    template <typename in_t, typename out_t>
    inline out_t Array3DAccessor<in_t, out_t>::get(const vec3i &where) const
    {
      return (out_t)actual->get(where);
    }

    template <typename in_t, typename out_t>
    inline size_t Array3DAccessor<in_t, out_t>::numElements() const
    {
      assert(actual);
      return actual->numElements();
    }

    // Array3DRepeater //

    template <typename T>
    inline Array3DRepeater<T>::Array3DRepeater(
        const std::shared_ptr<Array3D<T>> &actual, const vec3i &repeatedSize)
        : repeatedSize(repeatedSize), actual(actual)
    {
    }

    template <typename T>
    inline vec3i Array3DRepeater<T>::size() const
    {
      return repeatedSize;
    }

    template <typename T>
    inline T Array3DRepeater<T>::get(const vec3i &_where) const
    {
      vec3i where(_where.x % repeatedSize.x,
                  _where.y % repeatedSize.y,
                  _where.z % repeatedSize.z);

      if ((_where.x / repeatedSize.x) % 2)
        where.x = repeatedSize.x - 1 - where.x;
      if ((_where.y / repeatedSize.y) % 2)
        where.y = repeatedSize.y - 1 - where.y;
      if ((_where.z / repeatedSize.z) % 2)
        where.z = repeatedSize.z - 1 - where.z;

      return actual->get(where);
    }

    template <typename T>
    inline size_t Array3DRepeater<T>::numElements() const
    {
      return size_t(repeatedSize.x) * size_t(repeatedSize.y) *
             size_t(repeatedSize.z);
    }


  }  // ::ospcommon::array3D
}  // ::ospcommon
