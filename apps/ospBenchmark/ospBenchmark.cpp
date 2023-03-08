// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BaseFixture.h"
#include "rkcommon/utility/getEnvVar.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <windows.h>
#endif

#include <iostream>
#include <string>

namespace {
std::vector<const char *> cmdArg;
} // end namespace

// Test init/shutdown cycle time //////////////////////////////////////////////

static void ospInit_ospShutdown(benchmark::State &state)
{
  ospShutdown();

  for (auto _ : state) {
    // Those temp variable are needed because the call to ospInit changes the
    // number and order of arguments
    std::vector<const char *> cmdArgCpy(cmdArg);
    int argc = cmdArgCpy.size();
    ospInit(&argc, cmdArgCpy.data());
    ospShutdown();
  }
  state.SetItemsProcessed(state.iterations());

  std::vector<const char *> cmdArgCpy(cmdArg);
  int argc = cmdArgCpy.size();
  ospInit(&argc, cmdArgCpy.data());
}

BENCHMARK(ospInit_ospShutdown)->Unit(benchmark::kMillisecond);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
#ifdef _WIN32
  bool waitForKey = false;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    // detect standalone console: cursor at (0,0)?
    waitForKey = csbi.dwCursorPosition.X == 0 && csbi.dwCursorPosition.Y == 0;
  }
#endif

  // As there is no other way of customizing help output
  // we have to detect `--help` by ourselves
  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--help") {
      std::cout
          << "The ospBenchmark is a performance measurement tool that "
             "allows to assess the Intel OSPRay library performance on a target "
             "hardware. It consists of many workloads that can be listed with "
             "[--benchmark_list_tests=true] parameter and then selected for run "
             "with [--benchmark_filter=<regex>] parameter. All test assets like "
             "meshes or textures are generated procedurally during workload "
             "initialization phase. If a test renders a frame it is stored "
             "to an offscreen frame buffer."
          << std::endl
          << std::endl;

      std::cout
          << "After tests completion a results table is printed. Each "
             "table row corresponds to a single test or a single test "
             "repetition. Number of repetitions can be controlled with "
             "[--benchmark_repetitions=<num_repetitions>] parameter. Each "
             "repetition completely recreates frame buffer, world, renderer "
             "and camera. The table has the following columns:"
          << std::endl;

      std::cout << "  'Benchmark' - the name of a test" << std::endl;
      std::cout << "  'Time' - single iteration wall clock time" << std::endl;
      std::cout
          << "  'CPU' - single iteration CPU time spend by the main thread"
          << std::endl;
      std::cout
          << "  'Iterations' - number of rendered frames, determined adaptively "
             "to test for specified minimum time"
          << std::endl;
      std::cout << "  'UserCounters' - frames per second value" << std::endl
                << std::endl;

      std::cout << "The list of all supported parameters:" << std::endl;
    }
  }

  // save original cmdline args for multiple init/shutdown cycles
  for (int i = 0; i < argc; i++)
    cmdArg.push_back(argv[i]);

  ospInit(&argc, (const char **)argv);
  std::atexit(ospShutdown);

  auto DIR = utility::getEnvVar<std::string>("OSPRAY_BENCHMARK_IMG_DIR");
  BaseFixture::dumpFinalImageDir = DIR.value_or("");

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

#ifdef _WIN32
  if (waitForKey) {
    printf("\n\tpress any key to exit");
    _getch();
  }
#endif

  return 0;
}
