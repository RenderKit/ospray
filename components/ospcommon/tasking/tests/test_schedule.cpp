#define CATCH_CONFIG_MAIN
#include "../../testing/catch.hpp"
#include "../schedule.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

using ospcommon::tasking::schedule;

TEST_CASE("async")
{
  std::atomic<int> val{0};
  std::mutex mutex;
  std::condition_variable condition;

  auto *val_p  = &val;
  auto *cond_p = &condition;

  schedule([=](){
    *val_p = 1;
    cond_p->notify_all();
  });

  std::unique_lock<std::mutex> lock(mutex);
  auto now = std::chrono::system_clock::now();
  condition.wait_until(lock,
                       now + std::chrono::seconds(3),
                       [&](){ return val.load() == 1; });

  // NOTE(jda) - can't directly check std::atomic value due to compile error...
  int check_val = val;
  REQUIRE(check_val == 1);
}
