#pragma once

#include <string>
#include <vector>
#include <gtest/gtest.h>
#include <ospray/ospray.h>

class OSPRayEnvironment : public ::testing::Environment {
 private:
  bool dumpImg;
  std::string rendererType;
  std::string deviceType;
  std::string baselineDir;
  std::string failedDir;
  osp::vec2i imgSize;
  OSPDevice device;

 public:
  OSPRayEnvironment(int argc, char **argv);
  ~OSPRayEnvironment() {};

  bool GetDumpImg() const {
    return dumpImg;
  }
  std::string GetRendererType() const {
    return rendererType;
  }
  osp::vec2i GetImgSize() const {
    return imgSize;
  }
  std::string GetDeviceType() const {
    return deviceType;
  }
  std::string GetBaselineDir() const {
    return baselineDir;
  }
  std::string GetFailedDir() const {
    return failedDir;
  }

  void ParsArgs(int argc, char **argv);
  std::string GetStrArgValue(std::string *arg) const;
  int GetNumArgValue(std::string *arg) const;
};
