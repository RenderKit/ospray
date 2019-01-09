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

#include "../../testing/catch.hpp"

#include "../../box.h"
#include "../../vec.h"
#include "../Any.h"

using ospcommon::utility::Any;

// Helper types ///////////////////////////////////////////////////////////////

struct OSPObject_T {};
using OSPObject = OSPObject_T*;

// Helper functions ///////////////////////////////////////////////////////////

template <typename T>
inline void verify_value(const Any &v, const T &correctValue)
{
  REQUIRE(v.valid());
  REQUIRE(v.is<T>());
  REQUIRE(v.get<T>() == correctValue);
}

template <typename T>
inline void test_interface(T testValue, T testValue2)
{
  Any v;
  REQUIRE(!v.valid());

  SECTION("Can make valid by construction")
  {
    Any v2(testValue);
    verify_value<T>(v2, testValue);
  }

  SECTION("Can make valid by calling operator=()")
  {
    v = testValue;
    verify_value<T>(v, testValue);
  }

  SECTION("Can make valid by copy construction")
  {
    v = testValue;
    Any v2(v);
    verify_value<T>(v2, testValue);
  }

  SECTION("Two objects with same value are equal if constructed the same")
  {
    v = testValue;
    Any v2 = testValue;
    REQUIRE(v == v2);
  }

  SECTION("Two objects with same value are equal if assigned from another")
  {
    v = testValue;
    Any v2 = testValue2;
    v = v2;
    REQUIRE(v == v2);
  }

  SECTION("Two objects with different values are not equal")
  {
    v = testValue;
    Any v2 = testValue2;
    REQUIRE(v != v2);
  }
}

// Tests //////////////////////////////////////////////////////////////////////

TEST_CASE("Any 'int' type behavior", "[types]")
{
  test_interface<int>(5, 7);
}

TEST_CASE("Any 'float' type behavior", "[types]")
{
  test_interface<float>(1.f, 2.f);
}

TEST_CASE("Any 'bool' type behavior", "[types]")
{
  test_interface<bool>(true, false);
}

TEST_CASE("Any 'vec3f' type behavior", "[types]")
{
  test_interface<ospcommon::vec3f>({1.f, 1.f, 1.f}, {2.f, 3.f, 4.f});
}

TEST_CASE("Any 'vec2f' type behavior", "[types]")
{
  test_interface<ospcommon::vec2f>({1.f, 1.f}, {3.f, 4.f});
}

TEST_CASE("Any 'vec2i' type behavior", "[types]")
{
  test_interface<ospcommon::vec2i>({1, 1}, {3, 4});
}

TEST_CASE("Any 'box3f' type behavior", "[types]")
{
  test_interface<ospcommon::box3f>({{1.f, 1.f, 1.f}, {2.f, 2.f, 2.f}},
                                   {{3.f, 4.f, 5.f}, {6.f, 7.f, 8.f}});
}

TEST_CASE("Any 'string' type behavior", "[types]")
{
  test_interface<std::string>("Hello", "World");
}

TEST_CASE("Any 'OSPObject' type behavior", "[types]")
{
  // NOTE(jda) - we just need some phony pointer addresses to test Any,
  //             no need to hand it "real" OSPRay objects...
  void *val1 = nullptr;
  void *val2 = nullptr;
  val1 = &val1;
  val2 = &val2;

  test_interface<OSPObject>(static_cast<OSPObject>(val1),
                            static_cast<OSPObject>(val2));
}
