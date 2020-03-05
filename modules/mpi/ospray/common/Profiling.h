// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
