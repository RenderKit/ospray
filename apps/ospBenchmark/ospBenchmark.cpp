// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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
