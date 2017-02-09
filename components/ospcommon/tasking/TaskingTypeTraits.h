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

#include <cstdlib>
#include <type_traits>

namespace ospcommon {
  
  //NOTE(jda) - This checks at compile time if T implements the method
  //            'void T::operator()'.
  template <typename T>
  struct has_operator_method
  {
    template <class, class> class checker;

    template <typename C>
    static std::true_type test(checker<C, decltype(&C::operator())> *);

    template <typename C>
    static std::false_type test(...);

    using type = decltype(test<T>(nullptr));
    static const bool value = std::is_same<std::true_type, type>::value;
  };

  //NOTE(jda) - This checks at compile time if T implements the method
  //            'void T::operator(P taskIndex)', where P is an integral type
  //            (must be short, int, uint, or size_t) at compile-time. To be used
  //            inside a static_assert().
  template <typename T>
  struct has_operator_method_with_integral_param
  {
    template <typename P>
    using t_param    = void(T::*)(P) const;
    using byte_t     = unsigned char;
    using operator_t = decltype(&T::operator());

    using param_is_byte     = std::is_same<t_param<byte_t>  , operator_t>;
    using param_is_short    = std::is_same<t_param<short>   , operator_t>;
    using param_is_int      = std::is_same<t_param<int>     , operator_t>;
    using param_is_unsigned = std::is_same<t_param<unsigned>, operator_t>;
    using param_is_long     = std::is_same<t_param<long>    , operator_t>;
    using param_is_size_t   = std::is_same<t_param<size_t>  , operator_t>;

    static const bool value = has_operator_method<T>::value &&
      (param_is_byte::value     ||
       param_is_short::value    ||
       param_is_int::value      ||
       param_is_unsigned::value ||
       param_is_long::value     ||
       param_is_size_t::value);
  };

} // ::ospcommon
