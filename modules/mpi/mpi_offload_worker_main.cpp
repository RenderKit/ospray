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

#ifdef _WIN32
#  include <malloc.h>
#else
#  include <alloca.h>
#endif
#include "ospray/ospray.h"

int main(int ac, const char *av[])
{
  // always append "--osp:mpi" to args; multiple occurences are not harmful,
  // and we want to retain the original args (which could have been e.g.
  // "--osp:logoutput")
  int argc = ac+1;
  const char **argv = (const char **)alloca(argc * sizeof(void *));
  for(int i = 0; i < ac; i++)
    argv[i] = av[i];
  argv[ac] = "--osp:mpi";

  ospInit(&argc, argv);

  return 0;
}
