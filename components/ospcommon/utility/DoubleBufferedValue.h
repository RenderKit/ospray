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

#include <utility>

namespace ospcommon {
  namespace utility {

    /*! This class represents two values which are double buffered. This is
        useful if one thread wants to work on a piece of data while another
        "uses" it. Then at some point, the caller can swap() the front and
        back values, where front() and back() references will be exchanged.

        Example: A rendering thread wants to work on a framebuffer while a GUI
                 thread wants to continuously draw the latest complete
                 framebuffer. Once the new frame is ready, they are swapped.

        NOTE: This isn't thread safe! Any references to front() and back() must
              be synchronized with when swap() gets called.
     */
    template <typename T>
    class DoubleBufferedValue
    {
    public:

      // This assumes that T is default constructable. If you want to use this
      // abstraction with non default constructable types, you will need to add
      // additional constructors.
      DoubleBufferedValue()  = default;
      ~DoubleBufferedValue() = default;

      T& front();
      const T& front() const;

      T& back();
      const T& back() const;

      void swap();

    private:

      int front_value{0};
      int back_value{1};
      T values[2];
    };

    // Inlined members ////////////////////////////////////////////////////////

    template <typename T>
    inline T& DoubleBufferedValue<T>::front()
    {
      return values[front_value];
    }

    template <typename T>
    inline const T& DoubleBufferedValue<T>::front() const
    {
      return values[front_value];
    }

    template <typename T>
    inline T& DoubleBufferedValue<T>::back()
    {
      return values[back_value];
    }

    template <typename T>
    inline const T& DoubleBufferedValue<T>::back() const
    {
      return values[back_value];
    }

    template <typename T>
    inline void DoubleBufferedValue<T>::swap()
    {
      std::swap(front_value, back_value);
    }

  } // ::ospcommon::utility
} // ::ospcommon
