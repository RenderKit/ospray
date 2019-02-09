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

#include <iostream>
#include <sstream>

#include "../common.h"
#include "../TypeTraits.h"
#include "demangle.h"

namespace ospcommon {
  namespace utility {

    /* 'Any' implements a single item container which erases its type (can hold
     *  any value which is copyable). The value can be extracted successfully
     *  only if the correct type is queried for the held value, where an
     *  exception is thrown otherwise. Similar (but perhaps not identical to)
     *  'boost::any' or C++17's 'std::any'.
     *
     *  Example:
     *
     *      Any myAny = 1;                 // myAny is an 'int' w/ value of '1'
     *      int value = myAny.get<int>();  // get value of '1' out of myAny
     *      char bad  = myAny.get<char>(); // throws exception
     */
    struct Any
    {
      Any() = default;
      Any(const Any &copy);

      template<typename T>
      Any(T value);

      ~Any() = default;

      Any& operator=(const Any &rhs);

      template<typename T>
      Any& operator=(T rhs);

      bool operator==(const Any &rhs) const;
      bool operator!=(const Any &rhs) const;

      template<typename T>
      T& get();

      template<typename T>
      const T& get() const;

      template<typename T>
      bool is() const;

      bool valid() const;

      std::string toString() const;

    private:

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

        // NOTE(jda) - Use custom type trait to select a real implementation of
        //             isSame(), or one that always returns 'false' if the
        //             template type 'T' does not implement operator==() with
        //             itself.
        template <typename TYPE>
        inline traits::HasOperatorEquals<TYPE, bool>//<-- substitues to 'bool'
        isSameImpl(handle_base *other) const;

        template <typename TYPE>
        inline traits::NoOperatorEquals<TYPE, bool>//<-- substitutes to 'bool'
        isSameImpl(handle_base *other) const;
      };

      // Data members //

      std::unique_ptr<handle_base> currentValue;
    };

    // Inlined Any definitions ////////////////////////////////////////////////

    template<typename T>
    inline Any::Any(T value) :
      currentValue(new handle<typename std::remove_reference<T>::type>(
                     std::forward<T>(value)
                   ))
    {
      static_assert(std::is_copy_constructible<T>::value
                    && std::is_copy_assignable<T>::value,
                    "Any can only be constructed with copyable values!");
    }

    inline Any::Any(const Any &copy) :
      currentValue(copy.valid() ? copy.currentValue->clone() : nullptr)
    {
    }

    inline Any &Any::operator=(const Any &rhs)
    {
      Any temp(rhs);
      currentValue = std::move(temp.currentValue);
      return *this;
    }

    template<typename T>
    inline Any &Any::operator=(T rhs)
    {
      static_assert(std::is_copy_constructible<T>::value
                    && std::is_copy_assignable<T>::value,
                    "Any can only be assigned values which are copyable!");

      currentValue = std::unique_ptr<handle_base>(
        new handle<typename std::remove_reference<T>::type>(
          std::forward<T>(rhs)
        )
      );

      return *this;
    }

    inline bool Any::operator==(const Any &rhs) const
    {
      return currentValue->isSame(rhs.currentValue.get());
    }

    inline bool Any::operator!=(const Any &rhs) const
    {
      return !(*this == rhs);
    }

    template<typename T>
    inline T &Any::get()
    {
      if (!valid())
        throw std::runtime_error("Can't query value from an empty Any!");

      if (is<T>())
        return *(static_cast<T*>(currentValue->data()));
      else {
        std::stringstream msg;
        msg << "Incorrect type queried for Any!" << '\n';
        msg << "  queried type == "
            << nameOf<T>() << '\n';
        msg << "  current type == "
            << demangle(currentValue->valueTypeID().name()) << '\n';
        throw std::runtime_error(msg.str());
      }
    }

    template<typename T>
    inline const T &Any::get() const
    {
      if (!valid())
        throw std::runtime_error("Can't query value from an empty Any!");

      if (is<T>())
        return *(static_cast<T*>(currentValue->data()));
      else {
        std::stringstream msg;
        msg << "Incorrect type queried for Any!" << '\n';
        msg << "  queried type == "
            << nameOf<T>() << '\n';
        msg << "  current type == "
            << demangle(currentValue->valueTypeID().name()) << '\n';
        throw std::runtime_error(msg.str());
      }
    }

    template<typename T>
    inline bool Any::is() const
    {
      return valid() &&
             (typeid(T).hash_code() == currentValue->valueTypeID().hash_code());
    }

    inline bool Any::valid() const
    {
      return currentValue.get() != nullptr;
    }

    inline std::string Any::toString() const
    {
      std::stringstream retval;
      retval << "Any : (currently holds value of type) --> "
             << demangle(currentValue->valueTypeID().name());
      return retval.str();
    }

    template<typename T>
    inline Any::handle<T>::handle(T v) : value(std::move(v))
    {
    }

    template<typename T>
    inline Any::handle_base *Any::handle<T>::clone() const
    {
      return new handle<T>(value);
    }

    template<typename T>
    inline const std::type_info &Any::handle<T>::valueTypeID() const
    {
      return typeid(T);
    }

    template<typename T>
    inline bool Any::handle<T>::isSame(Any::handle_base *other) const
    {
      return isSameImpl<T>(other);
    }

    template <typename T>
    template <typename TYPE>
    inline traits::HasOperatorEquals<TYPE, bool>
    Any::handle<T>::isSameImpl(Any::handle_base *other) const
    {
      handle<T>* otherHandle = dynamic_cast<handle<T>*>(other);
      return (otherHandle != nullptr) && (otherHandle->value == this->value);
    }

    template <typename T>
    template <typename TYPE>
    inline traits::NoOperatorEquals<TYPE, bool>
    Any::handle<T>::isSameImpl(Any::handle_base *) const
    {
      return false;
    }

    template<typename T>
    inline void *Any::handle<T>::data()
    {
      return &value;
    }

  } // ::ospcommon::utility
} // ::ospcommon
