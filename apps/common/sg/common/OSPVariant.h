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

#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/box.h"

namespace ospray {
  namespace sg {

    using namespace ospcommon;

    struct OSPVariant
    {
      OSPVariant();

      template <typename T>
      OSPVariant(const T &value);

      ~OSPVariant();

      template <typename T>
      void set(const T& value);

      template <typename T>
      T get();

      template <typename T>
      bool is();

    private:

      // Helper types //

      enum class Type
      {
        OSPOBJECT, VEC3F, VEC2F, VEC2I, BOX3F, STRING, FLOAT, BOOL, INT, INVALID
      };

      // Helper functions //

      void cleanup();

      template <typename T>
      Type type();

      // Data members //

      Type currentType;

      union
      {
        OSPObject    as_OSPObject;
        vec3f        as_vec3f;
        vec2f        as_vec2f;
        vec2i        as_vec2i;
        box3f        as_box3f;
        std::string *as_string;
        float        as_float;
        bool         as_bool;
        int          as_int;
      } data;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline OSPVariant::OSPVariant()
    {
      type = Type::INVALID;
      data.as_string(nullptr);
    }

    template<typename T>
    inline OSPVariant::OSPVariant(const T &value)
    {
      set(value);
    }

    inline OSPVariant::~OSPVariant()
    {
      cleanup();
    }

    inline void OSPVariant::cleanup()
    {
      if (is<std::string>() && data.as_string != nullptr)
        delete data.as_string;
    }

    // OSPVariant::type() //

    template<typename T>
    inline OSPVariant::Type OSPVariant::type()
    {
      return Type::INVALID;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<OSPObject>()
    {
      return Type::OSPOBJECT;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec3f>()
    {
      return Type::VEC3F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec2f>()
    {
      return Type::VEC2F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec2i>()
    {
      return Type::VEC2I;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<box3f>()
    {
      return Type::BOX3F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<std::string>()
    {
      return Type::STRING;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<float>()
    {
      return Type::FLOAT;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<bool>()
    {
      return Type::BOOL;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<int>()
    {
      return Type::INT;
    }

    // OSPVariant::set() //

    template<typename T>
    inline void OSPVariant::set(const T &value)
    {
      throw std::runtime_error("Incompatible type given to OSPVariant::set()!");
    }

    template<>
    inline void OSPVariant::set(const OSPObject &value)
    {
      cleanup();
      data.as_OSPObject = value;
      currentType = type<OSPObject>();
    }

    template<>
    inline void OSPVariant::set(const vec3f &value)
    {
      cleanup();
      data.as_vec3f = value;
      currentType = type<vec3f>();
    }

    template<>
    inline void OSPVariant::set(const vec2f &value)
    {
      cleanup();
      data.as_vec2f = value;
      currentType = type<vec2f>();
    }

    template<>
    inline void OSPVariant::set(const vec2i &value)
    {
      cleanup();
      data.as_vec2i = value;
      currentType = type<vec2i>();
    }

    template<>
    inline void OSPVariant::set(const box3f &value)
    {
      cleanup();
      data.as_box3f = value;
      currentType = type<box3f>();
    }

    template<>
    inline void OSPVariant::set(const std::string &value)
    {
      cleanup();
      data.as_string  = new std::string;
      *data.as_string = value;
      currentType = type<std::string>();
    }

    template<>
    inline void OSPVariant::set(const float &value)
    {
      cleanup();
      data.as_float = value;
      currentType = type<float>();
    }

    template<>
    inline void OSPVariant::set(const bool &value)
    {
      cleanup();
      data.as_bool = value;
      currentType = type<bool>();
    }

    template<>
    inline void OSPVariant::set(const int &value)
    {
      cleanup();
      data.as_int = value;
      currentType = type<int>();
    }

    // OSPVariant::get() //

    template<typename T>
    inline T OSPVariant::get()
    {
      throw std::runtime_error("Incompatible type given to OSPVariant::get()!");
    }

    template<>
    inline OSPObject OSPVariant::get()
    {
      if (currentType == type<OSPObject>())
        return data.as_OSPObject;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline vec3f OSPVariant::get()
    {
      if (currentType == type<vec3f>())
        return data.as_vec3f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline vec2f OSPVariant::get()
    {
      if (currentType == type<vec2f>())
        return data.as_vec2f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline vec2i OSPVariant::get()
    {
      if (currentType == type<vec2i>())
        return data.as_vec2i;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline box3f OSPVariant::get()
    {
      if (currentType == type<box3f>())
        return data.as_box3f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline std::string OSPVariant::get()
    {
      if (currentType == type<std::string>())
        return *data.as_string;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline float OSPVariant::get()
    {
      if (currentType == type<float>())
        return data.as_float;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline bool OSPVariant::get()
    {
      if (currentType == type<bool>())
        return data.as_bool;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline int OSPVariant::get()
    {
      if (currentType == type<int>())
        return data.as_int;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    // OSPVariant::is() //

    template<typename T>
    inline bool OSPVariant::is()
    {
      return currentType == type<T>();
    }

  } // ::ospray::sg
} // ::ospray
