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

#include "../StringManip.h"

TEST_CASE("longestBeginningMatch() correctness", "[]")
{
  std::string input1 = "0123456";
  std::string input2 = "01234";

  auto output = ospcommon::utility::longestBeginningMatch(input1, input2);
  REQUIRE(output == "01234");
}

TEST_CASE("beginsWith() correctness", "[]")
{
  std::string input = "0123456";

  REQUIRE(ospcommon::utility::beginsWith(input, "012"));
  REQUIRE(!ospcommon::utility::beginsWith(input, "12"));
}

TEST_CASE("split() correctness", "[]")
{
  std::string input = "str0,str1,str2";

  auto output = ospcommon::utility::split(input, ',');
  REQUIRE(output[0] == "str0");
  REQUIRE(output[1] == "str1");
  REQUIRE(output[2] == "str2");
}

TEST_CASE("lowerCase() correctness", "[]")
{
  std::string input = "ABCd";

  auto output = ospcommon::utility::lowerCase(input);
  REQUIRE(output == "abcd");
}

TEST_CASE("upperCase() correctness", "[]")
{
  std::string input = "abcD";

  auto output = ospcommon::utility::upperCase(input);
  REQUIRE(output == "ABCD");
}