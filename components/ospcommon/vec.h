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

#include "common.h"
#include "constants.h"
#include "math.h"

#include "TypeTraits.h"

namespace ospcommon {

  struct vec_base {}; // NOTE: only for identifying vec_t types at compile-time

  // -------------------------------------------------------
  // type traits relevant to vec_t<> type compile-time logic
  // -------------------------------------------------------

  namespace traits {

    template <typename T>
    using is_arithmetic_t = enable_if_t<std::is_arithmetic<T>::value>;

    template <typename T>
    struct is_vec
    {
      const static bool value = std::is_base_of<vec_base, T>::value;
    };

    template <typename VEC_TYPE, typename SCALAR_TYPE>
    struct is_valid_scalar_for_binary_op
    {
      // NOTE: decay types to throw out pointerness and/or constness
      using vec_element_t =
          typename std::decay<typename VEC_TYPE::scalar_t>::type;
      using scalar_t = typename std::decay<SCALAR_TYPE>::type;

      const static bool value =
          std::is_constructible<vec_element_t, scalar_t>::value &&
          !is_vec<scalar_t>::value;
    };

    template <typename VEC_TYPE, typename SCALAR_TYPE>
    using is_valid_scalar_for_binary_op_t =
      enable_if_t<is_valid_scalar_for_binary_op<VEC_TYPE, SCALAR_TYPE>::value>;

    template <typename VEC_ELEMENT_T, typename TYPE_IN_QUESTION>
    struct is_valid_vec_constructor_type
    {
      const static bool value =
          std::is_constructible<VEC_ELEMENT_T, TYPE_IN_QUESTION>::value &&
          !std::is_same<TYPE_IN_QUESTION, VEC_ELEMENT_T>::value &&
          !is_vec<TYPE_IN_QUESTION>::value;
    };

    template <typename VEC_ELEMENT_T, typename TYPE_IN_QUESTION>
    using is_valid_vec_constructor_type_t =
        enable_if_t<
          is_valid_vec_constructor_type<VEC_ELEMENT_T, TYPE_IN_QUESTION>::value
        >;

  } // ::ospcommon::traits

  // vec_t<> types ////////////////////////////////////////////////////////////

  template <typename T, int N, bool ALIGN = false>
  struct vec_t : public vec_base
  {
    using scalar_t = T;
    using Scalar   = T;
  };

  template <typename T>
  struct vec_t<T, 2> : public vec_base
  {
    using scalar_t = T;
    using Scalar   = T;

    inline vec_t() = default;

    inline vec_t(scalar_t s) : x(s), y(s)
    {
    }

    template <typename OT,
              typename = traits::is_valid_vec_constructor_type_t<T, OT>>
    inline vec_t(const OT &s) : x(s), y(s)
    {
    }

    inline vec_t(scalar_t x, scalar_t y) : x(x), y(y)
    {
    }

    template <typename OT, bool OA>
    inline vec_t(const vec_t<OT, 2, OA> &o) : x(o.x), y(o.y)
    {
    }

    inline const T &operator[](const size_t idx) const
    {
      assert(idx < 2);
      return (&x)[idx];
    }

    inline T &operator[](const size_t idx)
    {
      assert(idx < 2);
      return (&x)[idx];
    }

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const
    {
      return x + y;
    }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const
    {
      return x * y;
    }

    T x, y;
  };

  template <typename T>
  struct vec_t<T, 3> : public vec_base
  {
    using scalar_t = T;
    using Scalar   = T;

    inline vec_t() = default;

    inline vec_t(scalar_t s) : x(s), y(s), z(s)
    {
    }

    template <typename OT,
              typename = traits::is_valid_vec_constructor_type_t<T, OT>>
    inline vec_t(const OT &s) : x(s), y(s), z(s)
    {
    }

    inline vec_t(scalar_t x, scalar_t y, scalar_t z) : x(x), y(y), z(z)
    {
    }

    template <typename OT, bool OA>
    inline vec_t(const vec_t<OT, 3, OA> &o) : x(o.x), y(o.y), z(o.z)
    {
    }

    inline const T &operator[](const size_t axis) const
    {
      assert(axis < 3);
      return (&x)[axis];
    }

    inline T &operator[](const size_t axis)
    {
      assert(axis < 3);
      return (&x)[axis];
    }

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const
    {
      return x + y + z;
    }

    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const
    {
      return x * y * z;
    }

    T x, y, z;
  };

  template <typename T>
  struct vec_t<T, 3, true> : public vec_base
  {
    using scalar_t = T;
    using Scalar   = T;

    inline vec_t() = default;

    inline vec_t(scalar_t s) : x(s), y(s), z(s)
    {
    }

    template <typename OT,
              typename = traits::is_valid_vec_constructor_type_t<T, OT>>
    inline vec_t(const OT &s) : x(s), y(s), z(s)
    {
    }

    inline vec_t(scalar_t x, scalar_t y, scalar_t z) : x(x), y(y), z(z)
    {
    }

    template <typename OT, bool OA>
    inline vec_t(const vec_t<OT, 3, OA> &o) : x(o.x), y(o.y), z(o.z)
    {
    }

    inline const T &operator[](const size_t axis) const
    {
      assert(axis < 3);
      return (&x)[axis];
    }
    inline T &operator[](const size_t axis)
    {
      assert(axis < 3);
      return (&x)[axis];
    }

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const
    {
      return x + y + z;
    }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const
    {
      return x * y * z;
    }

    inline operator vec_t<T, 3>() const
    {
      return vec_t<T, 3>(x, y, z);
    }

    T x, y, z;
    T padding_;
  };

  template <typename T>
  struct vec_t<T, 4> : public vec_base
  {
    using scalar_t = T;
    using Scalar   = T;

    inline vec_t() = default;

    inline vec_t(scalar_t s) : x(s), y(s), z(s), w(s)
    {
    }

#if 0
    template <typename OT,
              typename = traits::is_valid_vec_constructor_type_t<T, OT>>
    inline vec_t(const OT &s) : x(s), y(s), z(s), w(s)
    {
    }
#endif

    inline vec_t(scalar_t x, scalar_t y, scalar_t z, scalar_t w)
        : x(x), y(y), z(z), w(w)
    {
    }

    template <typename OT, typename WT>
    inline vec_t(const vec_t<OT, 3> &o, const WT w)
        : x(o.x), y(o.y), z(o.z), w(w)
    {
    }

    template <typename OT, bool OA>
    inline vec_t(const vec_t<OT, 4, OA> &o)
        : x(o.x), y(o.y), z(o.z), w(o.w)
    {
    }

    inline const T &operator[](const size_t idx) const
    {
      assert(idx < 4);
      return (&x)[idx];
    }
    inline T &operator[](const size_t idx)
    {
      assert(idx < 4);
      return (&x)[idx];
    }

    /*! return result of reduce_add() across all components */
    inline scalar_t sum() const
    {
      return x + y + z + w;
    }
    /*! return result of reduce_mul() across all components */
    inline scalar_t product() const
    {
      return x * y * z * w;
    }

    T x, y, z, w;
  };

  // -------------------------------------------------------
  // unary operators
  // -------------------------------------------------------
  template <typename T>
  inline vec_t<T, 2> operator-(const vec_t<T, 2> &v)
  {
    return vec_t<T, 2>(-v.x, -v.y);
  }
  template <typename T>
  inline vec_t<T, 3> operator-(const vec_t<T, 3> &v)
  {
    return vec_t<T, 3>(-v.x, -v.y, -v.z);
  }
  template <typename T>
  inline vec_t<T, 3, 1> operator-(const vec_t<T, 3, 1> &v)
  {
    return vec_t<T, 3, 1>(-v.x, -v.y, -v.z);
  }
  template <typename T>
  inline vec_t<T, 4> operator-(const vec_t<T, 4> &v)
  {
    return vec_t<T, 4>(-v.x, -v.y, -v.z, -v.w);
  }

  template <typename T>
  inline vec_t<T, 2> operator+(const vec_t<T, 2> &v)
  {
    return vec_t<T, 2>(+v.x, +v.y);
  }
  template <typename T>
  inline vec_t<T, 3> operator+(const vec_t<T, 3> &v)
  {
    return vec_t<T, 3>(+v.x, +v.y, +v.z);
  }
  template <typename T>
  inline vec_t<T, 3, 1> operator+(const vec_t<T, 3, 1> &v)
  {
    return vec_t<T, 3, 1>(+v.x, +v.y, +v.z);
  }
  template <typename T>
  inline vec_t<T, 4> operator+(const vec_t<T, 4> &v)
  {
    return vec_t<T, 4>(+v.x, +v.y, +v.z, +v.w);
  }

// -------------------------------------------------------
// unary functors
// -------------------------------------------------------
#define unary_functor(op)                                   \
  template <typename T>                                     \
  inline vec_t<T, 2> op(const vec_t<T, 2> &v)               \
  {                                                         \
    return vec_t<T, 2>(op(v.x), op(v.y));                   \
  }                                                         \
  template <typename T>                                     \
  inline vec_t<T, 3> op(const vec_t<T, 3> &v)               \
  {                                                         \
    return vec_t<T, 3>(op(v.x), op(v.y), op(v.z));          \
  }                                                         \
  template <typename T>                                     \
  inline vec_t<T, 3, true> op(const vec_t<T, 3, 1> &v)      \
  {                                                         \
    return vec_t<T, 3, 1>(op(v.x), op(v.y), op(v.z));       \
  }                                                         \
  template <typename T>                                     \
  inline vec_t<T, 4> op(const vec_t<T, 4> &v)               \
  {                                                         \
    return vec_t<T, 4>(op(v.x), op(v.y), op(v.z), op(v.w)); \
  }

  unary_functor(rcp)
  unary_functor(abs)
  unary_functor(sin)
  unary_functor(cos)
#undef unary_functor

// -------------------------------------------------------
// binary arithmetic operators
// -------------------------------------------------------

# define binary_operator(name, op)                                             \
  /* "vec op vec" */                                                           \
  template <typename T>                                                        \
  inline vec_t<T, 2> name(const vec_t<T, 2> &a, const vec_t<T, 2> &b)          \
  {                                                                            \
    return vec_t<T, 2>(a.x op b.x, a.y op b.y);                                \
  }                                                                            \
                                                                               \
  template <typename T, bool A, bool B>                                        \
  inline vec_t<T, 3> name(const vec_t<T, 3, A> &a, const vec_t<T, 3, B> &b)    \
  {                                                                            \
    return vec_t<T, 3>(a.x op b.x, a.y op b.y, a.z op b.z);                    \
  }                                                                            \
                                                                               \
  template <typename T>                                                        \
  inline vec_t<T, 4> name(const vec_t<T, 4> &a, const vec_t<T, 4> &b)          \
  {                                                                            \
    return vec_t<T, 4>(a.x op b.x, a.y op b.y, a.z op b.z, a.w op b.w);        \
  }                                                                            \
                                                                               \
  /* "vec<T, N> op vec<U, N>" (element types don't match) */                   \
  template <typename T1,                                                       \
            typename T2,                                                       \
            int N,                                                             \
            bool A,                                                            \
            typename = traits::is_not_same_t<T1, T2>>                          \
  inline auto name(const vec_t<T1, N, A> &v1, const vec_t<T2, N, A> &v2)       \
      -> vec_t<decltype(T1() op T2()), N, A>                                   \
  {                                                                            \
    using result_t = vec_t<decltype(T1() op T2()), N, A>;                      \
    return result_t(result_t(v1) op result_t(v2));                             \
  }                                                                            \
                                                                               \
  /* "vec op scalar" (SFINAE out scalar types which are ill-formed) */         \
  template <typename T,                                                        \
            typename U = T,                                                    \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 2>, U>>\
  inline auto name(const vec_t<T, 2> &a, const U &b)                           \
      -> vec_t<decltype(T() op U()), 2>                                        \
  {                                                                            \
    using result_t = vec_t<decltype(T() op U()), 2>;                           \
    return result_t(a.x op b, a.y op b);                                       \
  }                                                                            \
                                                                               \
  template <typename T,                                                        \
            bool A,                                                            \
            typename U = T,                                                    \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 3, A>, U>> \
  inline auto name(const vec_t<T, 3, A> &a, const U &b)                        \
      -> vec_t<decltype(T() op U()), 3, A>                                     \
  {                                                                            \
    using result_t = vec_t<decltype(T() op U()), 3, A>;                        \
    return result_t(a.x op b, a.y op b, a.z op b);                             \
  }                                                                            \
                                                                               \
  template <typename T,                                                        \
            typename U = T,                                                    \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 4>, U>>\
  inline auto name(const vec_t<T, 4> &a, const U &b)                           \
      -> vec_t<decltype(T() op U()), 4>                                        \
  {                                                                            \
    using result_t = vec_t<decltype(T() op U()), 4>;                           \
    return result_t(a.x op b, a.y op b, a.z op b, a.w op b);                   \
  }                                                                            \
                                                                               \
  /* "scalar op vec" (SFINAE out scalar types which are ill-formed) */         \
  template <typename T,                                                        \
            typename U = T,                                                     \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 2>, U>> \
  inline auto name(const U &a, const vec_t<T, 2> &b)                           \
      -> vec_t<decltype(U() op T()), 2>                                        \
  {                                                                            \
    using result_t = vec_t<decltype(U() op T()), 2>;                           \
    return result_t(a op b.x, a op b.y);                                       \
  }                                                                            \
                                                                               \
  template <typename T,                                                        \
            bool A,                                                            \
            typename U = T,                                                    \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 3, A>, U>> \
  inline auto name(const U &a, const vec_t<T, 3, A> &b)                        \
      -> vec_t<decltype(U() op T()), 3, A>                                     \
  {                                                                            \
    using result_t = vec_t<decltype(U() op T()), 3, A>;                        \
    return result_t(a op b.x, a op b.y, a op b.z);                             \
  }                                                                            \
                                                                               \
  template <typename T,                                                        \
            typename U = T,                                                    \
            typename = traits::is_valid_scalar_for_binary_op_t<vec_t<T, 4>, U>>\
  inline auto name(const U &a, const vec_t<T, 4> &b)                           \
      -> vec_t<decltype(U() op T()), 4>                                        \
  {                                                                            \
    using result_t = vec_t<decltype(U() op T()), 4>;                           \
    return result_t(a op b.x, a op b.y, a op b.z, a op b.w);                   \
  }

  binary_operator(operator+, +)
  binary_operator(operator-, -)
  binary_operator(operator*, *)
  binary_operator(operator/, /)
  binary_operator(operator%, %)
#undef binary_operator

// -------------------------------------------------------
// binary arithmetic assignment operators
// -------------------------------------------------------
#define binary_operator(name, op)                                         \
  /* "vec op vec" */                                                      \
  template <typename T, typename U>                                       \
  inline vec_t<T, 2> &name(vec_t<T, 2> &a, const vec_t<U, 2> &b)          \
  {                                                                       \
    a.x op b.x;                                                           \
    a.y op b.y;                                                           \
    return a;                                                             \
  }                                                                       \
                                                                          \
  template <typename T, typename U, bool A, bool B>                       \
  inline vec_t<T, 3, A> &name(vec_t<T, 3, A> &a, const vec_t<U, 3, B> &b) \
  {                                                                       \
    a.x op b.x;                                                           \
    a.y op b.y;                                                           \
    a.z op b.z;                                                           \
    return a;                                                             \
  }                                                                       \
                                                                          \
  template <typename T, typename U>                                       \
  inline vec_t<T, 4> &name(vec_t<T, 4> &a, const vec_t<U, 4> &b)          \
  {                                                                       \
    a.x op b.x;                                                           \
    a.y op b.y;                                                           \
    a.z op b.z;                                                           \
    a.w op b.w;                                                           \
    return a;                                                             \
  }                                                                       \
                                                                          \
  /* "vec op scalar" */                                                   \
  template <typename T,                                                   \
            typename U,                                                   \
            typename = traits::is_arithmetic_t<U>>                        \
  inline vec_t<T, 2> &name(vec_t<T, 2> &a, const U &b)                    \
  {                                                                       \
    a.x op b;                                                             \
    a.y op b;                                                             \
    return a;                                                             \
  }                                                                       \
                                                                          \
  template <typename T,                                                   \
            typename U,                                                   \
            bool A,                                                       \
            typename = traits::is_arithmetic_t<U>>                        \
  inline vec_t<T, 3, A> &name(vec_t<T, 3, A> &a, const U &b)              \
  {                                                                       \
    a.x op b;                                                             \
    a.y op b;                                                             \
    a.z op b;                                                             \
    return a;                                                             \
  }                                                                       \
                                                                          \
  template <typename T,                                                   \
            typename U,                                                   \
            typename = traits::is_arithmetic_t<U>>                        \
  inline vec_t<T, 4> &name(vec_t<T, 4> &a, const U &b)                    \
  {                                                                       \
    a.x op b;                                                             \
    a.y op b;                                                             \
    a.z op b;                                                             \
    a.w op b;                                                             \
    return a;                                                             \
  }

  binary_operator(operator+=, +=)
  binary_operator(operator-=, -=)
  binary_operator(operator*=, *=)
  binary_operator(operator/=, /=)
  binary_operator(operator%=, %=)
#undef binary_operator

  // -------------------------------------------------------
  // ternary operators (just for compatibility with old embree
  // -------------------------------------------------------
  template <typename T, bool A>
  inline vec_t<T, 3, A> madd(const vec_t<T, 3, A> &a,
                             const vec_t<T, 3, A> &b,
                             const vec_t<T, 3, A> &c)
  {
    return vec_t<T, 3, A>(
        madd(a.x, b.x, c.x), madd(a.y, b.y, c.y), madd(a.z, b.z, c.z));
  }

  // -------------------------------------------------------
  // comparison operators
  // -------------------------------------------------------
  template <typename T>
  inline bool operator==(const vec_t<T, 2> &a, const vec_t<T, 2> &b)
  {
    return a.x == b.x && a.y == b.y;
  }

  template <typename T, bool A, bool B>
  inline bool operator==(const vec_t<T, 3, A> &a, const vec_t<T, 3, B> &b)
  {
    return a.x == b.x && a.y == b.y && a.z == b.z;
  }

  template <typename T>
  inline bool operator==(const vec_t<T, 4> &a, const vec_t<T, 4> &b)
  {
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
  }

  template <typename T>
  inline bool operator!=(const vec_t<T, 2> &a, const vec_t<T, 2> &b)
  {
    return !(a == b);
  }

  template <typename T, bool A, bool B>
  inline bool operator!=(const vec_t<T, 3, A> &a, const vec_t<T, 3, B> &b)
  {
    return !(a == b);
  }

  template <typename T>
  inline bool operator!=(const vec_t<T, 4> &a, const vec_t<T, 4> &b)
  {
    return !(a == b);
  }

  // 'anyLessThan' - return true if any component is less than the other vec's
  template <typename T>
  inline bool anyLessThan(const vec_t<T, 2> &a, const vec_t<T, 2> &b)
  {
    return a.x < b.x || a.y < b.y;
  }

  template <typename T, bool A, bool B>
  inline bool anyLessThan(const vec_t<T, 3, A> &a, const vec_t<T, 3, B> &b)
  {
    return a.x < b.x || a.y < b.y || a.z < b.z;
  }

  template <typename T>
  inline bool anyLessThan(const vec_t<T, 4> &a, const vec_t<T, 4> &b)
  {
    return a.x < b.x || a.y < b.y || a.z < b.z || a.w < b.w;
  }

  // -------------------------------------------------------
  // dot functions
  // -------------------------------------------------------
  template <typename T>
  inline T dot(const vec_t<T, 2> &a, const vec_t<T, 2> &b)
  {
    return a.x * b.x + a.y * b.y;
  }
  template <typename T>
  inline T dot(const vec_t<T, 3> &a, const vec_t<T, 3> &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
  template <typename T>
  inline T dot(const vec_t<T, 3, 1> &a, const vec_t<T, 3, 1> &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
  template <typename T>
  inline T dot(const vec_t<T, 3> &a, const vec_t<T, 3, 1> &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
  template <typename T>
  inline T dot(const vec_t<T, 3, 1> &a, const vec_t<T, 3> &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z;
  }
  template <typename T>
  inline T dot(const vec_t<T, 4> &a, const vec_t<T, 4> &b)
  {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
  }

  // -------------------------------------------------------
  // length functions
  // -------------------------------------------------------
  template <typename T, int N, bool A>
  inline T length(const vec_t<T, N, A> &v)
  {
    return sqrt(dot(v, v));
  }

  // -------------------------------------------------------
  // cross product
  // -------------------------------------------------------
  template <typename T, bool A, bool B>
  inline vec_t<T, 3> cross(const vec_t<T, 3, A> &a, const vec_t<T, 3, B> &b)
  {
    return vec_t<T, 3>(
        a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
  }

  // -------------------------------------------------------
  // normalize()
  // -------------------------------------------------------
  template <typename T, int N, bool A>
  inline vec_t<T, N, A> normalize(const vec_t<T, N, A> &v)
  {
    return v * rsqrt(dot(v, v));
  }

  template <typename T, int N, bool A>
  inline vec_t<T, N, A> safe_normalize(const vec_t<T, N, A> &v)
  {
    return v * rsqrt(max(1e-6f, dot(v, v)));
  }

  // -------------------------------------------------------
  // interpolation
  // -------------------------------------------------------

  // barycentric interpolation
  template <typename T, int N, bool A>
  inline vec_t<T, N, A> interpolate_uv(const vec_t<float, 3> &f,
                                       const vec_t<T, N, A> &a,
                                       const vec_t<T, N, A> &b,
                                       const vec_t<T, N, A> &c)
  {
    return f.x*a + f.y*b + f.z*c;
  }

  // -------------------------------------------------------
  // ostream operators
  // -------------------------------------------------------
  template <typename T>
  inline std::ostream &operator<<(std::ostream &o, const vec_t<T, 2> &v)
  {
    o << "(" << v.x << "," << v.y << ")";
    return o;
  }
  template <typename T, bool A>
  inline std::ostream &operator<<(std::ostream &o, const vec_t<T, 3, A> &v)
  {
    o << "(" << v.x << "," << v.y << "," << v.z << ")";
    return o;
  }
  template <typename T>
  inline std::ostream &operator<<(std::ostream &o, const vec_t<T, 4> &v)
  {
    o << "(" << v.x << "," << v.y << "," << v.z << "," << v.w << ")";
    return o;
  }

  // "inherit" std::min/max/etc for basic types
  using std::min;
  using std::max;

// -------------------------------------------------------
// binary functors
// -------------------------------------------------------
#define define_functor(f)                                                   \
  template <typename T>                                                     \
  inline vec_t<T, 2> f(const vec_t<T, 2> &a, const vec_t<T, 2> &b)          \
  {                                                                         \
    return vec_t<T, 2>(f(a.x, b.x), f(a.y, b.y));                           \
  }                                                                         \
                                                                            \
  template <typename T, bool A>                                              \
  inline vec_t<T, 3, A> f(const vec_t<T, 3, A> &a, const vec_t<T, 3, A> &b) \
  {                                                                         \
    return vec_t<T, 3, A>(f(a.x, b.x), f(a.y, b.y), f(a.z, b.z));           \
  }                                                                         \
                                                                            \
  template <typename T>                                                     \
  inline vec_t<T, 4> f(const vec_t<T, 4> &a, const vec_t<T, 4> &b)          \
  {                                                                         \
    return vec_t<T, 4>(f(a.x, b.x), f(a.y, b.y), f(a.z, b.z), f(a.w, b.w)); \
  }

  define_functor(min)
  define_functor(max)
  define_functor(divRoundUp)
#undef define_functor

  // -------------------------------------------------------
  // reductions
  // -------------------------------------------------------
  template <typename T, bool A>
  inline T reduce_add(const vec_t<T, 2, A> &v)
  {
    return v.x + v.y;
  }
  template <typename T, bool A>
  inline T reduce_add(const vec_t<T, 3, A> &v)
  {
    return v.x + v.y + v.z;
  }
  template <typename T, bool A>
  inline T reduce_add(const vec_t<T, 4, A> &v)
  {
    return v.x + v.y + v.z + v.w;
  }

  template <typename T, bool A>
  inline T reduce_mul(const vec_t<T, 2, A> &v)
  {
    return v.x * v.y;
  }
  template <typename T, bool A>
  inline T reduce_mul(const vec_t<T, 3, A> &v)
  {
    return v.x * v.y * v.z;
  }
  template <typename T, bool A>
  inline T reduce_mul(const vec_t<T, 4, A> &v)
  {
    return v.x * v.y * v.z * v.w;
  }

  template <typename T, bool A>
  inline T reduce_min(const vec_t<T, 2, A> &v)
  {
    return min(v.x, v.y);
  }
  template <typename T, bool A>
  inline T reduce_min(const vec_t<T, 3, A> &v)
  {
    return min(min(v.x, v.y), v.z);
  }
  template <typename T, bool A>
  inline T reduce_min(const vec_t<T, 4, A> &v)
  {
    return min(min(v.x, v.y), min(v.z, v.w));
  }

  template <typename T, bool A>
  inline T reduce_max(const vec_t<T, 2, A> &v)
  {
    return max(v.x, v.y);
  }
  template <typename T, bool A>
  inline T reduce_max(const vec_t<T, 3, A> &v)
  {
    return max(max(v.x, v.y), v.z);
  }
  template <typename T, bool A>
  inline T reduce_max(const vec_t<T, 4, A> &v)
  {
    return max(max(v.x, v.y), max(v.z, v.w));
  }

  // -------------------------------------------------------
  // all vec2 variants
  // -------------------------------------------------------
  typedef vec_t<uint8_t, 2> vec2uc;
  typedef vec_t<int8_t, 2> vec2c;
  typedef vec_t<uint32_t, 2> vec2ui;
  typedef vec_t<int32_t, 2> vec2i;
  typedef vec_t<uint64_t, 2> vec2ul;
  typedef vec_t<int64_t, 2> vec2l;
  typedef vec_t<float, 2> vec2f;
  typedef vec_t<double, 2> vec2d;

  // -------------------------------------------------------
  // all vec3 variants
  // -------------------------------------------------------
  typedef vec_t<uint8_t, 3> vec3uc;
  typedef vec_t<int8_t, 3> vec3c;
  typedef vec_t<uint32_t, 3> vec3ui;
  typedef vec_t<int32_t, 3> vec3i;
  typedef vec_t<uint64_t, 3> vec3ul;
  typedef vec_t<int64_t, 3> vec3l;
  typedef vec_t<float, 3> vec3f;
  typedef vec_t<double, 3> vec3d;

  typedef vec_t<float, 3, 1> vec3fa;
  typedef vec_t<int, 3, 1> vec3ia;

  // -------------------------------------------------------
  // all vec4 variants
  // -------------------------------------------------------
  typedef vec_t<uint8_t, 4> vec4uc;
  typedef vec_t<int8_t, 4> vec4c;
  typedef vec_t<uint32_t, 4> vec4ui;
  typedef vec_t<int32_t, 4> vec4i;
  typedef vec_t<uint64_t, 4> vec4ul;
  typedef vec_t<int64_t, 4> vec4l;
  typedef vec_t<float, 4> vec4f;
  typedef vec_t<double, 4> vec4d;

  // -------------------------------------------------------
  // parsing from strings
  // -------------------------------------------------------
  OSPCOMMON_INTERFACE int toInt(const char *ptr);
  OSPCOMMON_INTERFACE float toFloat(const char *ptr);
  OSPCOMMON_INTERFACE vec2f toVec2f(const char *ptr);
  OSPCOMMON_INTERFACE vec3f toVec3f(const char *ptr);
  OSPCOMMON_INTERFACE vec4f toVec4f(const char *ptr);
  OSPCOMMON_INTERFACE vec2i toVec2i(const char *ptr);
  OSPCOMMON_INTERFACE vec3i toVec3i(const char *ptr);
  OSPCOMMON_INTERFACE vec4i toVec4i(const char *ptr);

  template <typename T, int N>
  inline size_t arg_max(const vec_t<T, N> &v)
  {
    size_t maxIdx = 0;
    for (size_t i = 1; i < N; i++)
      if (v[i] > v[maxIdx])
        maxIdx = i;
    return maxIdx;
  }

}  // ::ospcommon

/*! template specialization for std::less comparison operator;
 *  we need those to be able to put vec's in std::map etc @{ */
/* Defining just operator< is prone to bugs, because a definition of an
 * ordering of vectors is a bit arbitrary and depends on the context.
 * For example, in box::extend we certainly want the element-wise min/max and
 * not the std::min/std::max made applicable by vec3f::operator<.
*/
namespace std {
  template <typename T>
  struct less<ospcommon::vec_t<T, 2>>
  {
    inline bool operator()(const ospcommon::vec_t<T, 2> &a,
                           const ospcommon::vec_t<T, 2> &b) const
    {
      return (a.x < b.x) || ((a.x == b.x) && (a.y < b.y));
    }
  };

  template <typename T, bool A>
  struct less<ospcommon::vec_t<T, 3, A>>
  {
    inline bool operator()(const ospcommon::vec_t<T, 3, A> &a,
                           const ospcommon::vec_t<T, 3, A> &b) const
    {
      return (a.x < b.x) ||
             ((a.x == b.x) && ((a.y < b.y) || ((a.y == b.y) && (a.z < b.z))));
    }
  };

  template <typename T>
  struct less<ospcommon::vec_t<T, 4>>
  {
    inline bool operator()(const ospcommon::vec_t<T, 4> &a,
                           const ospcommon::vec_t<T, 4> &b) const
    {
      return (a.x < b.x) ||
             ((a.x == b.x) &&
              ((a.y < b.y) ||
               ((a.y == b.y) &&
                ((a.z < b.z) || ((a.z == b.z) && (a.w < b.w))))));
    }
  };

}  // ::ospcommon
