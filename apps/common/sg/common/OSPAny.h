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

#include "ospcommon/common.h"

namespace ospray {
  namespace sg {

    struct OSPAny
    {
      OSPAny() = default;
      OSPAny(const OSPAny &copy);

      template<typename T>
      OSPAny(T value);

      ~OSPAny() = default;

      OSPAny& operator=(const OSPAny &rhs);

      template<typename T>
      OSPAny& operator=(T rhs);

      template<typename T>
      const T& get() const;

      template<typename T>
      bool is() const;

      bool valid() const;

      std::string toString() const;

    private:

      // Friends //

      friend bool operator==(const OSPAny &lhs, const OSPAny &rhs);

      // Helper types //

      struct handle_base
      {
        virtual ~handle_base() = default;
        virtual handle_base* clone() const = 0;
        virtual const std::type_info& valueTypeID() const = 0;
        virtual bool isSame(handle_base *other) const = 0;
        virtual void *data() = 0;
      };

      template <typename T>
      struct handle : public handle_base
      {
        handle(T value);
        handle_base* clone() const override;
        const std::type_info& valueTypeID() const override;
        bool isSame(handle_base *other) const override;
        void *data() override;
        T value;
      };

      // Data members //

      std::unique_ptr<handle_base> currentValue;
    };

    // Inlined OSPAny definitions /////////////////////////////////////////////

    template<typename T>
    inline OSPAny::OSPAny(T value) :
      currentValue(new handle<typename std::remove_reference<T>::type>(
                     std::forward<T>(value)
                   ))
    {
    }

    inline OSPAny::OSPAny(const OSPAny &copy) :
      currentValue(copy.currentValue->clone())
    {
    }

    inline OSPAny &OSPAny::operator=(const OSPAny &rhs)
    {
      OSPAny temp(rhs);
      currentValue = std::move(temp.currentValue);
      return *this;
    }

    template<typename T>
    inline OSPAny &OSPAny::operator=(T rhs)
    {
      currentValue = std::unique_ptr<handle_base>(
        new handle<typename std::remove_reference<T>::type>(
          std::forward<T>(rhs)
        )
      );
      return *this;
    }

    template<typename T>
    const T &OSPAny::get() const
    {
      if (!valid())
        throw std::runtime_error("Can't query value from an empty OSPAny!");

      if (is<T>())
        return *(static_cast<T*>(currentValue->data()));
      else
        throw std::runtime_error("Incorrect type queried for OSPAny!");
    }

    template<typename T>
    bool OSPAny::is() const
    {
      return valid() &&
             (typeid(T).hash_code() == currentValue->valueTypeID().hash_code());
    }

    inline bool OSPAny::valid() const
    {
      return currentValue.get() != nullptr;
    }

    inline std::string OSPAny::toString() const
    {
      return "OSPAny<>";
    }

    template<typename T>
    inline OSPAny::handle<T>::handle(T v) : value(std::move(v))
    {
    }

    template<typename T>
    inline OSPAny::handle_base *OSPAny::handle<T>::clone() const
    {
      return new handle<T>(value);
    }

    template<typename T>
    inline const std::type_info &OSPAny::handle<T>::valueTypeID() const
    {
      return typeid(T);
    }

    template<typename T>
    inline bool OSPAny::handle<T>::isSame(OSPAny::handle_base *other) const
    {
      handle<T>* otherHandle = dynamic_cast<handle<T>*>(other);
      return (otherHandle != nullptr) && (otherHandle->value == this->value);
    }

    template<typename T>
    void *OSPAny::handle<T>::data()
    {
      return &value;
    }

    // Comparison function ////////////////////////////////////////////////////

    inline bool operator==(const OSPAny &lhs, const OSPAny &rhs)
    {
      return lhs.currentValue->isSame(rhs.currentValue.get());
    }

    inline bool operator!=(const OSPAny &lhs, const OSPAny &rhs)
    {
      return !(lhs == rhs);
    }

  } // ::ospray::sg
} // ::ospray
