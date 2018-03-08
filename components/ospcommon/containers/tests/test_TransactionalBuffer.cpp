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

#define private public
#include "../TransactionalBuffer.h"
#undef private

using ospcommon::TransactionalBuffer;

// Tests //////////////////////////////////////////////////////////////////////

TEST_CASE("Interface Tests", "[all]")
{
  TransactionalBuffer<int> tb;

  REQUIRE(tb.size() == 0);
  REQUIRE(tb.empty());

  tb.push_back(2);

  REQUIRE(tb.buffer.size() == 1);
  REQUIRE(tb.size() == 1);
  REQUIRE(!tb.empty());
  REQUIRE(tb.buffer[0] == 2);

  auto v = tb.consume();

  REQUIRE(v.size() == 1);
  REQUIRE(v[0] == 2);

  REQUIRE(tb.buffer.empty());
}
