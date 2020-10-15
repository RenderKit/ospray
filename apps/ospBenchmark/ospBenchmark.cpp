// Copyright 2018-2020 Intel Corporation
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

// Test init/shutdown cycle time //////////////////////////////////////////////

static void ospInit_ospShutdown(benchmark::State &state)
{
  ospShutdown();

  for (auto _ : state) {
    ospInit();
    ospShutdown();
  }
  state.SetItemsProcessed(state.iterations());

  ospInit();
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

  ospInit(&argc, (const char **)argv);

  auto DIR = utility::getEnvVar<std::string>("OSPRAY_BENCHMARK_IMG_DIR");
  BaseFixture::dumpFinalImageDir = DIR.value_or("");

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  ospShutdown();

#ifdef _WIN32
  if (waitForKey) {
    printf("\n\tpress any key to exit");
    _getch();
  }
#endif

  return 0;
}
