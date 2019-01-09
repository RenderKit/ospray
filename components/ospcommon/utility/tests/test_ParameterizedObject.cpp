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

#include "../ParameterizedObject.h"

using ospcommon::utility::ParameterizedObject;

TEST_CASE("ParameterizedObject correctness", "[]")
{
  ParameterizedObject obj;

  auto name = "test_int";

  obj.setParam(name, 5);

  REQUIRE(obj.hasParam(name));
  REQUIRE(obj.getParam<int>(name, 4) == 5);
  REQUIRE(obj.getParam<short>(name, 4) == 4);

  obj.removeParam(name);

  REQUIRE(!obj.hasParam(name));
  REQUIRE(obj.getParam<int>(name, 4) == 4);
  REQUIRE(obj.getParam<short>(name, 4) == 4);
}
