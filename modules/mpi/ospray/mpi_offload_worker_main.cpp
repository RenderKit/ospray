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

#include <cstring>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include "ospray/ospray.h"

int main(int argc, const char **av) {
  std::vector<const char*> args(av, av + argc);

  auto fnd = std::find_if(args.begin(), args.end(),
      [](const char *c) {
        return std::strcmp(c, "--osp:mpi") == 0
          || std::strcmp(c, "--osp:mpi-listen") == 0;
      });

  if (fnd == args.end())
    args.push_back("--osp:mpi");

  argc = args.size();
  ospInit(&argc, args.data());

  return 0;
}
