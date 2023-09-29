// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "OSPCommon.h"

#include <map>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifdef OBJECTFACTORY_IMPORT
#define OF_DECLSPEC __declspec(dllimport)
#else
#define OF_DECLSPEC __declspec(dllexport)
#endif
#else
#define OF_DECLSPEC
#endif

// 'fcns' member is not defined when imported from other library
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-var-template"

namespace ospray {

template <typename T, typename... Args>
struct OF_DECLSPEC ObjectFactory
{
  // Register a new type to instantiate
  template <typename D>
  static void registerType(const char *type)
  {
    fcns[type] = &allocate_object<D>;
  }

  // Instantiate provided type class
  static T *createInstance(const char *type, Args &&...args)
  {
    // Find constructor pointer and call it
    FactoryFcn fcn = fcns[type];
    if (fcn) {
      auto *obj = fcn(std::forward<Args>(args)...);
      if (obj != nullptr)
        return obj;
    }

    const auto type_string = stringFor(OSPTypeFor<T *>::value);
    throw std::runtime_error("Could not find '" + type_string + "' of type: '"
        + type
        + "'. Make sure you have the correct OSPRay libraries linked and initialized.");
  }

 private:
  using FactoryFcn = T *(*)(Args &&...args);
  using FactoryMap = std::map<std::string, FactoryFcn>;

  // Map of constructor pointers, preferably use 'inline static' since C++17
  static FactoryMap fcns;

  template <typename D>
  static T *allocate_object(Args &&...args)
  {
    return new D(std::forward<Args>(args)...);
  }
};

#ifndef OBJECTFACTORY_IMPORT
template <typename T, typename... Args>
typename ObjectFactory<T, Args...>::FactoryMap ObjectFactory<T, Args...>::fcns;
#endif
} // namespace ospray

#pragma clang diagnostic pop

#undef OF_DECLSPEC
