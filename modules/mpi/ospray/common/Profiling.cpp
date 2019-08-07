// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include <cstring>
#include <fstream>
#include <vector>
#include "Profiling.h"

namespace ospray {
  namespace mpi {
    using namespace std::chrono;

    ProfilingPoint::ProfilingPoint()
    {
#ifndef _WIN32
      std::memset(&usage, 0, sizeof(rusage));
      getrusage(RUSAGE_SELF, &usage);
#endif
      time = high_resolution_clock::now();
    }

    bool startsWith(const std::string &a, const std::string &prefix)
    {
      if (a.size() < prefix.size()) {
        return false;
      }
      return std::equal(prefix.begin(), prefix.end(), a.begin());
    }

    void logProfilingData(std::ostream &os, const ProfilingPoint &start,
                          const ProfilingPoint &end)
    {
#ifndef _WIN32
      const double elapsedCpu =
        end.usage.ru_utime.tv_sec + end.usage.ru_stime.tv_sec
        - (start.usage.ru_utime.tv_sec + start.usage.ru_stime.tv_sec)
        + 1e-6f * (end.usage.ru_utime.tv_usec + end.usage.ru_stime.tv_usec
            - (start.usage.ru_utime.tv_usec + start.usage.ru_stime.tv_usec));

      const double elapsedWall =
        duration_cast<duration<double>>(end.time - start.time).count();
      os << "\tCPU: " << elapsedCpu / elapsedWall * 100.0 << "%\n";

      std::ifstream procStatus("/proc/" + std::to_string(getpid()) + "/status");
      std::string line;
      const static std::vector<std::string> propPrefixes {
        "Threads", "Cpus_allowed_list", "VmSize", "VmRSS"
      };
      while (std::getline(procStatus, line)) {
        for (const auto &p : propPrefixes) {
          if (startsWith(line, p)) {
            os << "\t" << line << "\n";
            break;
          }
        }
      }
#endif
    }
  }
}

