#pragma once

#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <fstream>
#include <vector>
#include <ospray/ospray.h>
#include <gtest/gtest.h>

#include "ospray_environment.h"

using pixelColorValue = unsigned char;

const pixelColorValue pixelThreshold = 10;
const float errorRate = 0.01;

enum class OsprayStatus {
  Ok,
  Fail,
  Error,
};

// helper function to write the rendered image as PPM file
OsprayStatus writeImg(const std::string &fileName, const osp::vec2i &size, const uint32_t *pixel);
// helper function to compare gold image with current framebuffer render
OsprayStatus compareImgWithBaseline(const osp::vec2i &size, const uint32_t *testImg,const std::string &testName);



