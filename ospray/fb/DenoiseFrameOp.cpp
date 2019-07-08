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

#ifndef _WIN32
#include <sys/times.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#include <chrono>
#include <iostream>

#include "FrameOp.h"
#include "DenoiseFrameOp.h"

namespace ospray {
  using namespace std::chrono;
  namespace test {

    // TODO WILL: Remove this and push the profiling data into some
    // utility within ospcommon. (windows support?)
    struct ProfilingPoint {
#ifndef _WIN32
      rusage usage;
#endif
      std::chrono::high_resolution_clock::time_point time;

      ProfilingPoint()
      {
#ifndef _WIN32
        std::memset(&usage, 0, sizeof(rusage));
        getrusage(RUSAGE_SELF, &usage);
#endif
        time = high_resolution_clock::now();
      }
    };

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

  DenoiseFrameOp::DenoiseFrameOp()
    : device(oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT))
  {
    oidnSetDevice1b(device, "setAffinity", false);
    oidnCommitDevice(device);
    filter = oidnNewFilter(device, "RT");
  }

  DenoiseFrameOp::~DenoiseFrameOp()
  {
    oidnReleaseFilter(filter);
    oidnReleaseDevice(device);
  }

  void DenoiseFrameOp::process(FrameBufferView &fb)
  {
    if (fb.colorBufferFormat != OSP_FB_RGBA32F)
      throw std::runtime_error("DenoiseFrameOp must be used with an RGBA32F "
          "color format framebuffer!");

    test::ProfilingPoint start;

    if (!prevFb.colorBuffer || fb != prevFb) {
      float *fbColor = static_cast<float*>(fb.colorBuffer);
      oidnSetSharedFilterImage(filter, "color", fbColor,
                               OIDN_FORMAT_FLOAT3, fb.fbDims.x, fb.fbDims.y,
                               0, sizeof(float) * 4, 0);

      if (fb.normalBuffer)
        oidnSetSharedFilterImage(filter, "normal", fb.normalBuffer, 
                                 OIDN_FORMAT_FLOAT3, fb.fbDims.x, fb.fbDims.y,
                                 0, 0, 0);

      if (fb.albedoBuffer)
        oidnSetSharedFilterImage(filter, "albedo", fb.albedoBuffer, 
                                 OIDN_FORMAT_FLOAT3, fb.fbDims.x, fb.fbDims.y,
                                 0, 0, 0);

      oidnSetSharedFilterImage(filter, "output", fbColor,
                               OIDN_FORMAT_FLOAT3, fb.fbDims.x, fb.fbDims.y,
                               0, sizeof(float) * 4, 0);

      oidnSetFilter1b(filter, "hdr", false);

      test::ProfilingPoint commitstart;
      oidnCommitFilter(filter);
      test::ProfilingPoint commitend;
      std::cout << "Commit took "
        << duration_cast<milliseconds>(commitend.time - commitstart.time).count() << "ms\n";

      prevFb = fb;
    }
    test::ProfilingPoint startexec;
    oidnExecuteFilter(filter);
    test::ProfilingPoint execend;

    const char* errorMessage = nullptr;
    if (oidnGetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
    {
      std::cout << "OIDN ERROR " << errorMessage << "\n";
      throw std::runtime_error("Error running OIDN: " + std::string(errorMessage));
    }

    test::ProfilingPoint end;
    std::cout << "Denoising took "
      << duration_cast<milliseconds>(end.time - start.time).count() << "ms\n"
      << "Exec took "
      << duration_cast<milliseconds>(execend.time - startexec.time).count() << "ms\n";
    test::logProfilingData(std::cout, start, end);
    std::cout << "======\n";
  }

  std::string DenoiseFrameOp::toString() const
  {
    return "ospray::DenoiseFrameOp";
  }

  OSP_REGISTER_IMAGE_OP(DenoiseFrameOp, frame_denoise);
}

