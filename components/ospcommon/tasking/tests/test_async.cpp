#define CATCH_CONFIG_MAIN
#include "../../testing/catch.hpp"
#include "../async.h"

using ospcommon::tasking::async;

TEST_CASE("async")
{
  auto future = async([=](){ return 1; });
  int retvalue = future.get();
  REQUIRE(retvalue == 1);
}
