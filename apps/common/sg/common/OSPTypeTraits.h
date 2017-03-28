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

namespace ospray {
  namespace sg {
    namespace traits {

      template <typename T, typename Arg>
      std::true_type operator==(const T&, const Arg&);

      template <typename T, typename Arg = T>
      struct HasOperatorEqualsT
      {
        enum
        {
          value = !std::is_same<decltype(*(T*)(0) == *(Arg*)(0)),
                                std::true_type>::value
        };
      };

      template <typename T, typename TYPE>
      using HasOperatorEquals =
        typename std::enable_if<HasOperatorEqualsT<T>::value, TYPE>::type;

      template <typename T, typename TYPE>
      using NoOperatorEquals =
        typename std::enable_if<!HasOperatorEqualsT<T>::value, TYPE>::type;

    } // ::ospray::sg::traits
  } // ::ospray::sg
} // ::ospray
