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
#include "../parallel_foreach.h"

#include <algorithm>
#include <vector>

using ospcommon::tasking::parallel_foreach;

TEST_CASE("parallel_foreach")
{
  const size_t N_ELEMENTS = 1e8;

  const int bad_value  = 0;
  const int good_value = 1;

  std::vector<int> v(N_ELEMENTS);
  std::fill(v.begin(), v.end(), bad_value);

  parallel_foreach(v, [&](int &value) {
    value = good_value;
  });

  auto found = std::find(v.begin(), v.end(), bad_value);

  REQUIRE(found == v.end());
}
