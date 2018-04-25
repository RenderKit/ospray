// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "../../testing/catch.hpp"

#include "../ArrayView.h"

#include <numeric>

using ospcommon::utility::ArrayView;

// Helper functions ///////////////////////////////////////////////////////////

template <typename T>
inline void verify_empty(const ArrayView<T> &v)
{
  // NOTE: implicitly tests data(), size(), begin(), cbegin(), and
  //       operator bool()
  REQUIRE(!v);
  REQUIRE(v.data() == nullptr);
  REQUIRE(v.size() == 0);
  REQUIRE(v.begin() == nullptr);
  REQUIRE(v.cbegin() == nullptr);
}

template <typename T>
inline void verify_N(const ArrayView<T> &v, int N)
{
  REQUIRE(N > 0);
  // NOTE: implicitly tests data(), size(), begin(), cbegin(), operator bool(),
  //       and operator[]
  REQUIRE(v);
  REQUIRE(v.data() != nullptr);
  REQUIRE(v.begin() != nullptr);
  REQUIRE(v.cbegin() != nullptr);
  REQUIRE(v.size() == size_t(N));

  for (int i = 0; i < N; ++i)
    REQUIRE(v[i] == i);
}

template <typename T, int SIZE>
inline std::array<T, SIZE> make_test_array()
{
  std::array<T, SIZE> a;
  std::iota(a.begin(), a.end(), 0);
  return a;
}

template <typename T>
inline std::vector<T> make_test_vector(int N)
{
  std::vector<T> v(N);
  std::iota(v.begin(), v.end(), 0);
  return v;
}

// Tests //////////////////////////////////////////////////////////////////////

TEST_CASE("ArrayView construction", "[constructors]")
{
  ArrayView<int> view;
  verify_empty(view);

  SECTION("Construct from a std::array")
  {
    auto array = make_test_array<int, 5>();
    ArrayView<int> view2(array);
    verify_N(view2, 5);

    SECTION("Assign from std::array")
    {
      view = array;
      verify_N(view, 5);
    }
  }

  SECTION("Construct from a std::vector")
  {
    auto vector = make_test_vector<int>(5);
    ArrayView<int> view2(vector);
    verify_N(view2, 5);

    SECTION("Assign from std::vector")
    {
      view = vector;
      verify_N(view, 5);
    }
  }
}

TEST_CASE("ArrayView::reset", "[methods]")
{
  ArrayView<int> view;
  auto vector = make_test_vector<int>(5);
  view = vector;
  verify_N(view, 5);

  SECTION("ArrayView::reset with no arguments")
  {
    view.reset();
    verify_empty(view);
  }

  SECTION("ArrayView::reset with arguements")
  {
    auto array = make_test_array<int, 10>();
    view.reset(array.data(), array.size());
    verify_N(view, 10);
  }
}