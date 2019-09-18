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

#pragma once

#ifndef _WIN32
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <chrono>
#include <ostream>

namespace ospray {
namespace mpi {

struct ProfilingPoint
{
#ifndef _WIN32
  rusage usage;
#endif
  std::chrono::high_resolution_clock::time_point time;

  ProfilingPoint();
};

void logProfilingData(
    std::ostream &os, const ProfilingPoint &start, const ProfilingPoint &end);
} // namespace mpi
} // namespace ospray
