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

#include "OSPCommon.h"

namespace ospray {

  inline size_t rdtsc() { return ospcommon::rdtsc(); }

  /*! function for "pretty" large numbers, printing 10000000 as "10M" instead */
  inline std::string prettyNumber(const size_t s) {
    double val = s;
    char result[100];
    if (val >= 1e12f) {
      sprintf(result,"%.1fT",val/1e12f);
    } else if (val >= 1e9f) {
      sprintf(result,"%.1fG",val/1e9f);
    } else if (val >= 1e6f) {
      sprintf(result,"%.1fM",val/1e6f);
    } else if (val >= 1e3f) {
      sprintf(result,"%.1fK",val/1e3f);
    } else {
      sprintf(result,"%lu",s);
    }
    return result;
  }

  template <typename T>
  inline std::pair<bool, T> getEnvVar(const std::string &/*var*/)
  {
    static_assert(!std::is_same<T, float>::value &&
                  !std::is_same<T, int>::value &&
                  !std::is_same<T, std::string>::value,
                  "You can only get an int, float, or std::string "
                  "when using ospray::getEnvVar<T>()!");
    return {false, {}};
  }

  template <>
  inline std::pair<bool, float>
  getEnvVar<float>(const std::string &var)
  {
    auto *str = getenv(var.c_str());
    bool found = (str != nullptr);
    return {found, found ? atof(str) : float{}};
  }

  template <>
  inline std::pair<bool, int>
  getEnvVar<int>(const std::string &var)
  {
    auto *str = getenv(var.c_str());
    bool found = (str != nullptr);
    return {found, found ? atoi(str) : int{}};
  }

  template <>
  inline std::pair<bool, std::string>
  getEnvVar<std::string>(const std::string &var)
  {
    auto *str = getenv(var.c_str());
    bool found = (str != nullptr);
    return {found, found ? std::string(str) : std::string{}};
  }

}// namespace ospray

