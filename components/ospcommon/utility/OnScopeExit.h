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

#include "../TypeTraits.h"
#include <functional>

namespace ospcommon {
  namespace utility {

    /* Execute a given function when a scope exits */
    struct OnScopeExit
    {
      template <typename FCN_T>
      OnScopeExit(FCN_T&& _fcn);

      ~OnScopeExit();

    private:

      std::function<void()> fcn;
    };

    // Inlined OnScopeExit definitions ////////////////////////////////////////

    template <typename FCN_T>
    inline OnScopeExit::OnScopeExit(FCN_T&& _fcn)
    {
      static_assert(traits::has_operator_method<FCN_T>::value,
                    "FCN_T must implement operator() with no arguments!");

      fcn = std::forward<FCN_T>(_fcn);
    }

    inline OnScopeExit::~OnScopeExit()
    {
      fcn();
    }

  } // ::ospcommon::utility
} // ::ospcommon
