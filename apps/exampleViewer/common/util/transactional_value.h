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

#include <mutex>

#include "ImguiUtilExport.h"

template <typename T>
class transactional_value
{
public:

  transactional_value()  = default;
  ~transactional_value() = default;

  template <typename OtherType>
  transactional_value(const OtherType &ot);

  template <typename OtherType>
  transactional_value& operator=(const OtherType &ot);

  transactional_value<T>& operator=(const transactional_value<T>& fp);

  T &ref();
  T  get();

  bool update();

private:

  bool newValue {false};
  T queuedValue;
  T currentValue;

  std::mutex mutex;
};

// Inlined transactional_value Members ////////////////////////////////////////

template <typename T>
template <typename OtherType>
inline transactional_value<T>::transactional_value(const OtherType &ot)
{
  currentValue = ot;
}

template <typename T>
template <typename OtherType>
inline transactional_value<T> &
transactional_value<T>::operator=(const OtherType &ot)
{
  std::lock_guard<std::mutex> lock{mutex};
  queuedValue = ot;
  newValue    = true;
  return *this;
}

template <typename T>
inline transactional_value<T> &
transactional_value<T>::operator=(const transactional_value<T> &fp)
{
  std::lock_guard<std::mutex> lock{mutex};
  queuedValue = fp.ref();
  newValue    = true;
  return *this;
}

template<typename T>
inline T &transactional_value<T>::ref()
{
  return currentValue;
}

template<typename T>
inline T transactional_value<T>::get()
{
  return currentValue;
}

template<typename T>
inline bool transactional_value<T>::update()
{
  bool didUpdate = false;
  if (newValue) {
    std::lock_guard<std::mutex> lock{mutex};
    currentValue = std::move(queuedValue);
    newValue     = false;
    didUpdate    = true;
  }

  return didUpdate;
}
