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

#include "OSPCommon.h"

#include <map>

namespace ospray {

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

  template <typename OSPRAY_CLASS, OSPDataType OSP_TYPE>
  inline OSPRAY_CLASS *createInstanceHelper(const std::string &type)
  {
    // Function pointer type for creating a concrete instance of a subtype of
    // this class.
    using creationFunctionPointer = OSPRAY_CLASS*(*)();

    // Function pointers corresponding to each subtype.
    static std::map<std::string, creationFunctionPointer> symbolRegistry;

    // Find the creation function for the subtype if not already known.
    if (symbolRegistry.count(type) == 0) {

      if (ospray::logLevel() >= 2)  {
        std::cout << "#ospray: trying to look up object type '"
                  << type << "' for the first time" << std::endl;
      }

      auto type_string = stringForType(OSP_TYPE);

      // Construct the name of the creation function to look for.
      std::string creationFunctionName = "ospray_create_" + type_string
                                         +  "__" + type;

      // Look for the named function.
      symbolRegistry[type] =
          (creationFunctionPointer)getSymbol(creationFunctionName);

      // The named function may not be found if the requested subtype is not
      // known.
      if (!symbolRegistry[type] && ospray::logLevel() >= 1) {
        std::cerr << "  WARNING: unrecognized object type '" + type
                  << "'." << std::endl;
      }
    }

    // Create a concrete instance of the requested subtype.
    auto *object = symbolRegistry[type] ? (*symbolRegistry[type])() : nullptr;

    // Denote the subclass type in the ManagedObject base class.
    if (object) {
      object->managedObjectType = OSP_TYPE;
    }
    else {
      symbolRegistry.erase(type);
      throw std::runtime_error("Could not find object of type: " 
        + type + ".  Make sure you have the correct OSPRay libraries linked.");
    }

    return object;
  }

}// namespace ospray

