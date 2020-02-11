// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "ospray/ospray_cpp.h"
using namespace ospray;

#include "ospcommon/math/vec.h"
using namespace ospcommon::math;

class OSPRayEnvironment : public ::testing::Environment
{
 public:
  OSPRayEnvironment(int argc, char **argv);
  ~OSPRayEnvironment() = default;

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

  void ParsArgs(int argc, char **argv);
  std::string GetStrArgValue(std::string *arg) const;
  int GetNumArgValue(std::string *arg) const;

 private:
  bool dumpImg;
  std::string rendererType;
  std::string baselineDir;
  std::string failedDir;
  vec2i imgSize{1024, 768};
};
