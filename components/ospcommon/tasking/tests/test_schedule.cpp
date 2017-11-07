#define CATCH_CONFIG_MAIN
#include "../../testing/catch.hpp"
#include "../schedule.h"

#include <atomic>

using ospcommon::tasking::schedule;

TEST_CASE("async")
{
  std::atomic<int> val{0};

  auto *val_p  = &val;

  schedule([=](){
    *val_p = 1;
  });

  while (val.load() == 0);

  REQUIRE(val.load() == 1);
}
