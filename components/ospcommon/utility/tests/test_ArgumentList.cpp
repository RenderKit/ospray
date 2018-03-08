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

#include "../ArgumentList.h"

using ospcommon::utility::ArgumentList;

static const char *test_arguments[5] =
    {"testApp", "arg1", "arg2", "arg3", "arg4"};

TEST_CASE("ArgumentList correctness", "[]")
{
  ArgumentList args(5, test_arguments);

  REQUIRE(!args.empty());

  REQUIRE(args.size() == 4);
  REQUIRE(args[0] != "testApp");

  args.remove(0);
  REQUIRE(args[0] == "arg2");
  REQUIRE(args[1] == "arg3");
  REQUIRE(args[2] == "arg4");

  args.remove(0, 2);
  REQUIRE(args[0] == "arg4");

  args.remove(0);
  REQUIRE(args.empty());
}

struct TestParser final : public ospcommon::utility::ArgumentsParser
{
  int tryConsume(ArgumentList &argList, int argID) override
  {
    const auto arg = argList[argID];
    if (arg == "arg1")
      return 1;
    else if (arg == "arg3")
      return 2;

    return 0;
  }
};

TEST_CASE("ArgumentsParser correctness", "[]")
{
  ArgumentList args(5, test_arguments);

  TestParser parser;
  parser.parseAndRemove(args);

  REQUIRE(args.size() == 1);
  REQUIRE(args[0] == "arg2");
}
