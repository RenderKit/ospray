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

#include <sstream>

namespace ospray {
  namespace sg {

    using namespace ospcommon;

    struct OSPVariant
    {
      OSPVariant();
      OSPVariant(const OSPVariant &copy);

      template<typename T>
      OSPVariant(const T &value);

      ~OSPVariant();

      template<typename T>
      void set(const T &value);

      template<typename T>
      OSPVariant& operator=(const T &rhs);

      template<typename T>
      const T& get() const;

      template<typename T>
      bool is() const;

      bool valid() const;

    private:

      // Friends //

      friend bool operator==(const OSPVariant &lhs, const OSPVariant &rhs);

      // Helper types //

      enum class Type
      {
        OSPOBJECT, VEC3F, VEC2F, VEC2I, BOX3F, STRING, FLOAT, BOOL, INT, INVALID
      };

      // Helper functions //

      void cleanup();

      template<typename T>
      Type type() const;

      // Data members //

      Type currentType {Type::INVALID};

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

    // OSPVariant::type() //

    template<typename T>
    inline OSPVariant::Type OSPVariant::type() const
    {
      return Type::INVALID;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<OSPObject>() const
    {
      return Type::OSPOBJECT;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec3f>() const
    {
      return Type::VEC3F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec2f>() const
    {
      return Type::VEC2F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<vec2i>() const
    {
      return Type::VEC2I;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<box3f>() const
    {
      return Type::BOX3F;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<std::string>() const
    {
      return Type::STRING;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<float>() const
    {
      return Type::FLOAT;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<bool>() const
    {
      return Type::BOOL;
    }

    template<>
    inline OSPVariant::Type OSPVariant::type<int>() const
    {
      return Type::INT;
    }

    // OSPVariant::get() //

    template<typename T>
    inline const T& OSPVariant::get() const
    {
      throw std::runtime_error("Incompatible type given to OSPVariant::get()!");
    }

    template<>
    inline const OSPObject& OSPVariant::get() const
    {
      if (currentType == type<OSPObject>())
        return data.as_OSPObject;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const vec3f& OSPVariant::get() const
    {
      if (currentType == type<vec3f>())
        return data.as_vec3f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const vec2f& OSPVariant::get() const
    {
      if (currentType == type<vec2f>())
        return data.as_vec2f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const vec2i& OSPVariant::get() const
    {
      if (currentType == type<vec2i>())
        return data.as_vec2i;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const box3f& OSPVariant::get() const
    {
      if (currentType == type<box3f>())
        return data.as_box3f;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const std::string& OSPVariant::get() const
    {
      if (currentType == type<std::string>())
        return *data.as_string;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const float& OSPVariant::get() const
    {
      if (currentType == type<float>())
        return data.as_float;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const bool& OSPVariant::get() const
    {
      if (currentType == type<bool>())
        return data.as_bool;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    template<>
    inline const int& OSPVariant::get() const
    {
      if (currentType == type<int>())
        return data.as_int;
      else
        throw std::runtime_error("Incorrect type queried from OSPVariant!");
    }

    // OSPVariant::set() //

    template<typename T>
    inline void OSPVariant::set(const T &value)
    {
      std::stringstream msg;
      msg << "Incompatible type given to OSPVariant::set()!" << std::endl;
      msg << "    type given: " << typeid(T).name() << std::endl;
      throw std::runtime_error(msg.str());
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

    template<>
    inline void OSPVariant::set(const OSPVariant &value)
    {
      cleanup();
      if (value.currentType == Type::STRING)
        set<std::string>(value.get<std::string>());
      else {
        currentType = value.currentType;
        data        = value.data;
      }
    }

    // OSPVariant::is() //

    template<typename T>
    inline bool OSPVariant::is() const
    {
      return currentType == type<T>();
    }

    inline bool OSPVariant::valid() const
    {
      return currentType != Type::INVALID;
    }

    // Other inlined definitions //////////////////////////////////////////////

    inline OSPVariant::OSPVariant()
    {
      std::memset(&data, 0, sizeof(data));
    }

    inline OSPVariant::OSPVariant(const OSPVariant &copy)
    {
      std::memset(&data, 0, sizeof(data));
      set(copy);
    }

    template<typename T>
    inline OSPVariant::OSPVariant(const T &value)
    {
      std::memset(&data, 0, sizeof(data));
      set(value);
    }

    inline OSPVariant::~OSPVariant()
    {
      cleanup();
    }

    template<typename T>
    inline OSPVariant& OSPVariant::operator=(const T &rhs)
    {
      set<T>(rhs);
    }

    inline void OSPVariant::cleanup()
    {
      if (is<std::string>() && data.as_string != nullptr)
        delete data.as_string;
      std::memset(&data, 0, sizeof(data));
    }

    // Overloaded operator definitions ////////////////////////////////////////

    inline bool operator==(const OSPVariant &lhs, const OSPVariant &rhs)
    {
      if(lhs.currentType != rhs.currentType);
        return false;

      if (lhs.is<std::string>())
        return lhs.get<std::string>() == rhs.get<std::string>();
      else if (lhs.is<OSPObject>())
        return lhs.get<OSPObject>() == rhs.get<OSPObject>();
      else if (lhs.is<vec3f>())
        return lhs.get<vec3f>() == rhs.get<vec3f>();
      else if (lhs.is<vec2f>())
        return lhs.get<vec2f>() == rhs.get<vec2f>();
      else if (lhs.is<vec2i>())
        return lhs.get<vec2i>() == rhs.get<vec2i>();
      else if (lhs.is<box3f>())
        return lhs.get<box3f>() == rhs.get<box3f>();
      else if (lhs.is<float>())
        return lhs.get<float>() == rhs.get<float>();
      else if (lhs.is<bool>())
        return lhs.get<bool>() == rhs.get<bool>();
      else
        return lhs.get<int>() == rhs.get<int>();
    }

    inline bool operator!=(const OSPVariant &lhs, const OSPVariant &rhs)
    {
      return !(lhs == rhs);
    }

  } // ::ospray::sg
} // ::ospray
