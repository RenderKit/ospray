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
