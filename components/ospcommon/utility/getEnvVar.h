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

#include "Optional.h"

namespace ospcommon {
  namespace utility {

    template <typename T>
    inline Optional<T> getEnvVar(const std::string &/*var*/)
    {
      static_assert(!std::is_same<T, float>::value &&
                    !std::is_same<T, int>::value &&
                    !std::is_same<T, std::string>::value,
                    "You can only get an int, float, or std::string "
                    "when using ospray::getEnvVar<T>()!");
      return {};
    }

    template <>
    inline Optional<float> getEnvVar<float>(const std::string &var)
    {
      auto *str = getenv(var.c_str());
      bool found = (str != nullptr);
      return found ? Optional<float>((float)atof(str)) : Optional<float>();
    }

    template <>
    inline Optional<int> getEnvVar<int>(const std::string &var)
    {
      auto *str = getenv(var.c_str());
      bool found = (str != nullptr);
      return found ? Optional<int>(atoi(str)) : Optional<int>();
    }

    template <>
    inline Optional<std::string> getEnvVar<std::string>(const std::string &var)
    {
      auto *str = getenv(var.c_str());
      bool found = (str != nullptr);
      return found ? Optional<std::string>(std::string(str)) :
                     Optional<std::string>();
    }

  } // ::ospcommon::utility
} // ::ospcommon
