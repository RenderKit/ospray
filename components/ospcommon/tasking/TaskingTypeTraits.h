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
    using T_SHORT_PARAM    = void(T::*)(short)        const;
    using T_INT_PARAM      = void(T::*)(int)          const;
    using T_UNSIGNED_PARAM = void(T::*)(unsigned int) const;
    using T_SIZET_PARAM    = void(T::*)(size_t)       const;

    using PARAM_IS_SHORT    = std::is_same<T_SHORT_PARAM,
                                           decltype(&T::operator())>;
    using PARAM_IS_INT      = std::is_same<T_INT_PARAM,
                                           decltype(&T::operator())>;
    using PARAM_IS_UNSIGNED = std::is_same<T_UNSIGNED_PARAM,
                                           decltype(&T::operator())>;
    using PARAM_IS_SIZET    = std::is_same<T_SIZET_PARAM,
                                           decltype(&T::operator())>;

    static const bool value = has_operator_method<T>::value &&
      (PARAM_IS_SHORT::value
       || PARAM_IS_INT::value
       || PARAM_IS_UNSIGNED::value
       || PARAM_IS_SIZET::value);
  };

} // ::ospcommon
