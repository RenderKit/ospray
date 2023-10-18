// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ospray/ospray_cpp.h"
#include "ospray/ospray_cpp/ext/rkcommon.h"
using namespace ospray;
using namespace rkcommon::math;

namespace sycl {
inline namespace _V1 {
class queue;
}
} // namespace sycl

class OSPRayEnvironment : public ::testing::Environment
{
 public:
  OSPRayEnvironment(int argc, char **argv);
  ~OSPRayEnvironment()
#ifndef SYCL_LANGUAGE_VERSION
      = default
#endif
      ;

  bool GetDumpImg() const
  {
    return dumpImg;
  }
  std::string GetRendererType() const
  {
    return rendererType;
  }
  vec2i GetImgSize() const
  {
    return imgSize;
  }
  std::string GetBaselineDir() const
  {
    return baselineDir;
  }
  std::string GetFailedDir() const
  {
    return failedDir;
  }

#ifdef SYCL_LANGUAGE_VERSION
  sycl::queue *GetAppsSyclQueue();
#endif

  void ParseArgs(int argc, char **argv);
  std::string GetStrArgValue(std::string *arg) const;
  int GetNumArgValue(std::string *arg) const;

 private:
  bool dumpImg{false};
  std::string rendererType{"scivis"};
  std::string baselineDir{"regression_test_baseline"};
  std::string failedDir{false};
  vec2i imgSize{1024, 768};
  bool ownSYCL{false};
#ifdef SYCL_LANGUAGE_VERSION
  sycl::queue *appsSyclQueue{nullptr};
#endif
};
