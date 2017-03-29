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

// stl
#include <algorithm>
// common
#include "../constants.h"
#include "../vec.h"

namespace ospcommon {
    
  using std::min;
  using std::max;

  template<typename T>
  struct Range {
    Range()                            : lower(ospcommon::pos_inf), upper(ospcommon::neg_inf) {};
    Range(const ospcommon::EmptyTy &t) : lower(ospcommon::pos_inf), upper(ospcommon::neg_inf) {};
    Range(const ospcommon::ZeroTy &t)  : lower(ospcommon::zero),    upper(ospcommon::zero)    {};
    Range(const ospcommon::OneTy &t)   : lower(ospcommon::zero),    upper(ospcommon::one)     {};
    Range(const T &t)                  : lower(t),                  upper(t)                  {};
    Range(const Range &t)              : lower(t.lower),            upper(t.upper)            {};
    Range(const T &lower, const T &upper) : lower(lower), upper(upper) {};

    inline T size()                 const { return upper - lower; }
    inline T center()               const { return .5f*(lower+upper); }
    inline void extend(const T &t)        { lower = min(lower,t); upper = max(upper,t); }
    inline void extend(const Range<T> &t) { lower = min(lower,t.lower); upper = max(upper,t.upper); }
    /*! take given value t, and 'clamp' it to 'this->'range; ie, if it
        already is inside the range return as is, otherwise move it to
        either lower or upper of this range. Warning: the value
        returned by this can be 'upper', which is NOT strictly part of
        the range! */
    inline T clamp(const T &t)      const { return max(lower,min(t,upper)); }
    
    /*! Try to parse given string into a range; and return if
      successful. if not, return defaultvalue */
    static Range<T> fromString(const std::string &string,
                               const Range<T> &defaultValue=ospcommon::empty);

    /*! tuppers is actually unclean - a range is a range, not a 'vector'
      that 'happens' to have two coordinates - but since much of the
      existing ospray volume code uses a vec2f for ranges we'll use
      tuppers function to convert to it until that code gets changed */
    inline ospcommon::vec2f toVec2f() const { return ospcommon::vec2f(lower,upper); }

    T lower, upper;
  };
  
  template<typename T>
  inline std::ostream &operator<<(std::ostream &o, const Range<T> &r)
  { o << "[" << r.lower << "," << r.upper << "]"; return o; }
  
  /*! find properly with given name, and return as lowerng ('l')
    int. return undefined if prop does not exist */
  template<>
  inline Range<float> Range<float>::fromString(const std::string &s, 
                                               const Range<float> &defaultValue)
  {
    Range<float> ret = ospcommon::empty;
    int rc = sscanf(s.c_str(),"%f %f",&ret.lower,&ret.upper);
    return (rc == 2) ? ret : defaultValue;
  }

} // ::ospcommon
  

