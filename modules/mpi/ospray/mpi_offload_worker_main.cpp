// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "ospray/ospray.h"

int main(int argc, const char **av)
{
  std::vector<const char *> args(av, av + argc);
  args.push_back("--osp:load-modules=mpi");
  args.push_back("--osp:device=mpiOffload");

  argc = args.size();
  ospInit(&argc, args.data());

  return 0;
}
