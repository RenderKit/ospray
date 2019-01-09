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

// stl
#include <algorithm>
// common
#include "constants.h"
#include "vec.h"

namespace ospcommon {

  using std::min;
  using std::max;

  /*! default implementatoin of 'anyLessThan' for scalar types, so we
      can make a range<float>s etc. Vec-types will overwrite that and
      test if _any_ dimension is less */
  template <typename TA, typename TB>
  inline bool anyLessThan(const TA &a, const TB &b)
  {
     return a < b;
  }

  template <typename T>
  struct range_t
  {
    range_t() : lower(ospcommon::pos_inf), upper(ospcommon::neg_inf){}
    range_t(const ospcommon::EmptyTy &)
        : lower(ospcommon::pos_inf), upper(ospcommon::neg_inf){}
    range_t(const ospcommon::ZeroTy &)
        : lower(ospcommon::zero), upper(ospcommon::zero){}
    range_t(const ospcommon::OneTy &)
        : lower(ospcommon::zero), upper(ospcommon::one){}
    range_t(const T &t) : lower(t), upper(t){}
    range_t(const T &_lower, const T &_upper) : lower(_lower), upper(_upper){}

    template <typename other_t>
    explicit range_t(const range_t<other_t> &other) 
      : lower(T(other.lower)), 
        upper(T(other.upper)) 
    {}

    inline T size() const
    {
      return upper - lower;
    }

    inline T center() const
    {
      return .5f * (lower + upper);
    }

    inline void extend(const T &t)
    {
      lower = min(lower, t);
      upper = max(upper, t);
    }

    inline void extend(const range_t<T> &t)
    {
      lower = min(lower, t.lower);
      upper = max(upper, t.upper);
    }

    /*! take given value t, and 'clamp' it to 'this->'range; ie, if it
        already is inside the range return as is, otherwise move it to
        either lower or upper of this range. Warning: the value
        returned by this can be 'upper', which is NOT strictly part of
        the range! */
    inline T clamp(const T &t) const
    {
      return max(lower, min(t, upper));
    }

    /*! Try to parse given string into a range; and return if
      successful. if not, return defaultvalue */
    static range_t<T> fromString(
      const std::string &string,
      const range_t<T> &defaultValue = ospcommon::empty
    );

    /*! tuppers is actually unclean - a range is a range, not a 'vector'
      that 'happens' to have two coordinates - but since much of the
      existing ospray volume code uses a vec2f for ranges we'll use
      tuppers function to convert to it until that code gets changed */
    inline ospcommon::vec2f toVec2f() const
    {
      return ospcommon::vec2f(lower, upper);
    }

    inline bool empty() const
    {
      return anyLessThan(upper, lower);
    }

    inline bool contains(const T &vec) const
    {
      return !anyLessThan(vec, lower) && !anyLessThan(upper, vec);
    }

    T lower, upper;
  };

  template <typename T>
  inline std::ostream &operator<<(std::ostream &o, const range_t<T> &r)
  {
    o << "[" << r.lower << "," << r.upper << "]";
    return o;
  }

  /*! scale range, per dimension */
  template<typename T>
  inline range_t<T> operator*(const range_t<T> &range, const T &scale)
  { return range_t<T>(range.lower*scale,range.upper*scale); }

  /*! scale range, per dimension */
  template<typename T>
  inline range_t<T> operator*(const T &scale, const range_t<T> &range)
  { return range_t<T>(range.lower*scale,range.upper*scale); }

  /*! translate a range, per dimension */
  template<typename T>
  inline range_t<T> operator+(const range_t<T> &range, const T &translation)
  { return range_t<T>(range.lower+translation,range.upper+translation); }

  /*! translate a range, per dimension */
  template<typename T>
  inline range_t<T> operator+(const T &translation, const range_t<T> &range)
  { return range_t<T>(range.lower+translation,range.upper+translation); }

  /*! find properly with given name, and return as lowerng ('l')
    int. return undefined if prop does not exist */
  template <>
  inline range_t<float> range_t<float>::fromString(
    const std::string &s,
    const range_t<float> &defaultValue
  )
  {
    range_t<float> ret = ospcommon::empty;
    int rc             = sscanf(s.c_str(), "%f %f", &ret.lower, &ret.upper);
    return (rc == 2) ? ret : defaultValue;
  }

  // range_t aliases //////////////////////////////////////////////////////////

  using range1f = range_t<float>;

}  // ::ospcommon
