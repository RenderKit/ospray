// Copyright 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BaseFixture.h"
// ospcommon
#include "ospcommon/utility/getEnvVar.h"

// Test init/shutdown cycle time //////////////////////////////////////////////

static void ospInit_ospShutdown(benchmark::State &state)
{
  ospShutdown();

  for (auto _ : state) {
    ospInit();
    ospShutdown();
  }

  ospInit();
}

BENCHMARK(ospInit_ospShutdown)->Unit(benchmark::kMillisecond);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  ospInit(&argc, (const char **)argv);

  auto DIR = utility::getEnvVar<std::string>("OSPRAY_BENCHMARK_IMG_DIR");
  BaseFixture::dumpFinalImageDir = DIR.value_or("");

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  ospShutdown();

  return 0;
}
