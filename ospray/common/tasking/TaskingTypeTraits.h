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

#include <cstdlib>
#include <type_traits>

namespace ospray {


//NOTE(jda) - This checks if T implements void T::operator(P taskIndex), where
//            P is an integral type (must be short, int, uint, or size_t)
template <typename T>
struct has_operator_method_integral_param
{
  template <class, class> class checker;

  using T_SHORT_PARAM    = void(T::*)(short)        const;
  using T_INT_PARAM      = void(T::*)(int)          const;
  using T_UNSIGNED_PARAM = void(T::*)(unsigned int) const;
  using T_SIZET_PARAM    = void(T::*)(size_t)       const;

  using IS_SHORT    = std::is_same<T_SHORT_PARAM,    decltype(&T::operator())>;
  using IS_INT      = std::is_same<T_INT_PARAM,      decltype(&T::operator())>;
  using IS_UNSIGNED = std::is_same<T_UNSIGNED_PARAM, decltype(&T::operator())>;
  using IS_SIZET    = std::is_same<T_SIZET_PARAM,    decltype(&T::operator())>;

  template <typename C>
  static std::true_type test(checker<C, decltype(&C::operator())> *);

  template <typename C>
  static std::false_type test(...);

  using type = decltype(test<T>(nullptr));
  static constexpr bool value = std::is_same<std::true_type, type>::value &&
                                (IS_SHORT::value
                                 || IS_INT::value
                                 || IS_UNSIGNED::value
                                 || IS_SIZET::value);
};

}//namespace ospray
