#define CATCH_CONFIG_MAIN
#include "ospcommon/testing/catch.hpp"

#include "OSPVariant.h"

using ospray::sg::OSPVariant;

// Helper functions ///////////////////////////////////////////////////////////

template <typename T>
inline void verify_value(const OSPVariant &v, const T &correctValue)
{
  REQUIRE(v.valid());
  REQUIRE(v.is<T>());
  REQUIRE(v.get<T>() == correctValue);
}

template <typename T>
inline void test_interface(T testValue, T testValue2)
{
  OSPVariant v;
  REQUIRE(!v.valid());

  SECTION("Can make valid by calling set()")
  {
    v.set(testValue);
    verify_value<T>(v, testValue);
  }

  SECTION("Can make valid by calling operator=()")
  {
    v = testValue;
    verify_value<T>(v, testValue);
  }

  SECTION("Two objects with same value are equal")
  {
    v = testValue;
    OSPVariant v2 = testValue;
    REQUIRE(v == v2);
  }

  SECTION("Two objects with different values are not equal")
  {
    v = testValue;
    OSPVariant v2 = testValue2;
    REQUIRE(v != v2);
  }
}

// Tests //////////////////////////////////////////////////////////////////////

TEST_CASE("OSPVariant 'int' type behavior", "[types]")
{
  test_interface<int>(5, 7);
}

TEST_CASE("OSPVariant 'float' type behavior", "[types]")
{
  test_interface<float>(1.f, 2.f);
}

TEST_CASE("OSPVariant 'bool' type behavior", "[types]")
{
  test_interface<bool>(true, false);
}

TEST_CASE("OSPVariant 'vec3f' type behavior", "[types]")
{
  test_interface<ospcommon::vec3f>({1.f, 1.f, 1.f}, {2.f, 3.f, 4.f});
}

TEST_CASE("OSPVariant 'vec2f' type behavior", "[types]")
{
  test_interface<ospcommon::vec2f>({1.f, 1.f}, {3.f, 4.f});
}

TEST_CASE("OSPVariant 'vec2i' type behavior", "[types]")
{
  test_interface<ospcommon::vec2i>({1, 1}, {3, 4});
}

TEST_CASE("OSPVariant 'box3f' type behavior", "[types]")
{
  test_interface<ospcommon::box3f>({{1.f, 1.f, 1.f}, {2.f, 2.f, 2.f}},
                                   {{3.f, 4.f, 5.f}, {6.f, 7.f, 8.f}});
}

TEST_CASE("OSPVariant 'string' type behavior", "[types]")
{
  test_interface<std::string>("Hello", "World");
}
