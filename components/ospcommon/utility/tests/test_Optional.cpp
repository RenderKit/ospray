#define CATCH_CONFIG_MAIN
#include "../../testing/catch.hpp"

#include "../Optional.h"

using ospcommon::utility::Optional;

// Helper functions ///////////////////////////////////////////////////////////

template <typename T>
inline void verify_no_value(const Optional<T> &opt)
{
  // Implicitly tests 'operator bool()' and 'has_value()' functions
  REQUIRE(!opt);
  REQUIRE(!opt.has_value());
}

template <typename T>
inline void verify_value(const Optional<T> &opt, const T &value)
{
  // Implicitly tests 'operator bool()', 'has_value()', and 'operator*()'
  REQUIRE(opt);
  REQUIRE(opt.has_value());
  REQUIRE(opt.value() == value);
  REQUIRE(*opt == value);
}

// Tests //////////////////////////////////////////////////////////////////////

TEST_CASE("Optional<> construction", "[constructors]")
{
  // Default construction results in "no value"
  Optional<int> default_opt;
  verify_no_value(default_opt);

  // All types that are not Optional<> construct w/ value
  Optional<int> value_opt(5);
  verify_value(value_opt, 5);

  SECTION("Copy construct from Optional<> without value")
  {
    Optional<int> copy_opt(default_opt);
    verify_no_value(copy_opt);
  }

  SECTION("Copy construct from Optional<> with value")
  {
    Optional<int> copy_opt(value_opt);
    verify_value(copy_opt, 5);
  }

  SECTION("Copy construct from Optional<> with value and different type")
  {
    Optional<float> copy_opt(value_opt);
    verify_value(copy_opt, 5.f);
  }

  SECTION("Move construct from Optional<> without value")
  {
    Optional<int> copy_opt(std::move(default_opt));
    verify_no_value(copy_opt);
  }

  SECTION("Move construct from Optional<> with value")
  {
    Optional<int> copy_opt(std::move(value_opt));
    verify_value(copy_opt, 5);
  }

  SECTION("Move construct from Optional<> with value and different type")
  {
    Optional<float> copy_opt(std::move(value_opt));
    verify_value(copy_opt, 5.f);
  }
}

TEST_CASE("Optional<>::operator=(T)", "[methods]")
{
  Optional<int> opt;
  opt = 5;
  verify_value(opt, 5);
}

TEST_CASE("Optional<>::operator=(Optional<>)", "[methods]")
{
  Optional<int> opt;
  opt = 5;

  Optional<int> otherOpt = opt;
  verify_value(otherOpt, 5);

  Optional<float> movedFromOpt;
  movedFromOpt = std::move(opt);
  verify_value(movedFromOpt, 5.f);
}

TEST_CASE("Optional<>::value_or()", "[methods]")
{
  Optional<int> opt;

  SECTION("Use default value in 'value_or()' if no value is present")
  {
    REQUIRE(opt.value_or(10) == 10);
  }

  SECTION("Use correct value in 'value_or()' if value is present")
  {
    opt = 5;
    REQUIRE(opt.value_or(10) == 5);
  }
}

TEST_CASE("Optional<>::reset()", "[methods]")
{
  Optional<int> opt(5);
  opt.reset();
  verify_no_value(opt);
}

TEST_CASE("Optional<>::emplace()", "[methods]")
{
  Optional<int> opt;
  opt.emplace(5);
  verify_value(opt, 5);
}

TEST_CASE("logical operators", "[free functions]")
{
  Optional<int> opt1(5);

  SECTION("operator==()")
  {
    Optional<int> opt2(5);
    REQUIRE(opt1 == opt2);
  }

  SECTION("operator!=()")
  {
    Optional<int> opt2(1);
    REQUIRE(opt1 != opt2);
  }

  SECTION("operator<()")
  {
    Optional<int> opt2(1);
    REQUIRE(opt2 < opt1);
  }

  SECTION("operator<=()")
  {
    Optional<int> opt2(5);
    REQUIRE(opt2 <= opt1);

    Optional<int> opt3(4);
    REQUIRE(opt3 <= opt2);
  }

  SECTION("operator>()")
  {
    Optional<int> opt2(1);
    REQUIRE(opt1 > opt2);
  }

  SECTION("operator>=()")
  {
    Optional<int> opt2(5);
    REQUIRE(opt1 >= opt2);

    Optional<int> opt3(4);
    REQUIRE(opt2 >= opt3);
  }
}

TEST_CASE("make_optional<>()", "[free functions]")
{
  Optional<int> opt1(5);
  auto opt2 = ospcommon::utility::make_optional<int>(5);
  REQUIRE(opt1 == opt2);
}