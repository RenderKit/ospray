// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Profiling.h"
#include <cstring>
#include <fstream>

namespace mpicommon {

using namespace std::chrono;

ProfilingPoint::ProfilingPoint()
{
#ifdef __linux__
  std::memset(&usage, 0, sizeof(rusage));
  getrusage(RUSAGE_SELF, &usage);
#endif
  time = steady_clock::now();
}

float cpuUtilization(const ProfilingPoint &start, const ProfilingPoint &end)
{
#ifdef __linux__
  const double elapsed_cpu = end.usage.ru_utime.tv_sec
      + end.usage.ru_stime.tv_sec
      - (start.usage.ru_utime.tv_sec + start.usage.ru_stime.tv_sec)
      + 1e-6f
          * (end.usage.ru_utime.tv_usec + end.usage.ru_stime.tv_usec
              - (start.usage.ru_utime.tv_usec + start.usage.ru_stime.tv_usec));

  const double elapsed_wall =
      duration_cast<duration<double>>(end.time - start.time).count();
  return elapsed_cpu / elapsed_wall * 100.0;
#else
  return -1.f;
#endif
}

size_t elapsedTimeMs(const ProfilingPoint &start, const ProfilingPoint &end)
{
  return duration_cast<milliseconds>(end.time - start.time).count();
}

std::string getProcStatus()
{
  // Note: this file doesn't exist on OS X, would we want some alternative to
  // fetch this info?
  std::ifstream file("/proc/self/status");
  if (!file.is_open()) {
    return "";
  }
  return std::string(
      std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
} // namespace mpicommon
