// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef ENABLE_PROFILING

#include <chrono>
#include <string>
#ifdef __linux__
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "ospray_mpi_common_export.h"

namespace mpicommon {

struct OSPRAY_MPI_COMMON_EXPORT ProfilingPoint
{
#ifdef __linux__
  rusage usage;
#endif
  std::chrono::steady_clock::time_point time;

  ProfilingPoint();
};

float cpuUtilization(const ProfilingPoint &start, const ProfilingPoint &end);

size_t elapsedTimeMs(const ProfilingPoint &start, const ProfilingPoint &end);

std::string getProcStatus();

} // namespace mpicommon

#endif

