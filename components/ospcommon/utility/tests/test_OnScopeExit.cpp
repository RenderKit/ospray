#define CATCH_CONFIG_MAIN
#include "../../testing/catch.hpp"

#include "../OnScopeExit.h"

using ospcommon::utility::OnScopeExit;

TEST_CASE("OnScopeExit correctness", "[]")
{
  int testValue = 0;

  {
    OnScopeExit guard([&](){ testValue++; });

    REQUIRE(testValue == 0);// hasn't executed yet
  }

  REQUIRE(testValue == 1);// executed exactly once?
}
